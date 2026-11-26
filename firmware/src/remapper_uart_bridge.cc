#include <cstdio>
#include <cstring>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "pio_usb.h"
#include "tusb.h"
#include "uart_bridge.h"

#define BRIDGE_UART uart0
#define UART_TX_PIN 0 // GP0 -> nice!nano RX (P0.08)
#define UART_RX_PIN 1 // GP1 <- nice!nano TX (P0.06)

#define USB_PORT1_DP 2 // Socket 1: D+ = GP2, D- = GP3
#define USB_PORT2_DP 4 // Socket 2: D+ = GP4, D- = GP5
#define USB_PORT3_DP 6 // Socket 3: D+ = GP6, D- = GP7

// Offset interface IDs by 0x1000 so they never collide with nRF52840 BLE conn_idx << 8 (0x0000..0x0700)
static inline uint16_t make_interface_id(uint8_t dev_addr, uint8_t instance) {
    return (uint16_t)(0x1000 | ((dev_addr & 0x0F) << 4) | (instance & 0x0F));
}

static void uart_send_frame(BridgeMsgType type, const uint8_t* payload, uint16_t len) {
    bridge_header_t hdr = {
        .magic0 = UART_BRIDGE_MAGIC0,
        .magic1 = UART_BRIDGE_MAGIC1,
        .type = type,
        .len = len,
    };
    uint8_t crc = bridge_crc8((const uint8_t*)&hdr.type, sizeof(hdr.type) + sizeof(hdr.len));
    for (uint16_t i = 0; i < len; i++) {
        crc ^= payload[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    uart_write_blocking(BRIDGE_UART, (const uint8_t*)&hdr, sizeof(hdr));
    if (len > 0) {
        uart_write_blocking(BRIDGE_UART, payload, len);
    }
    uart_write_blocking(BRIDGE_UART, &crc, 1);
}

// Core 1 runs Pico-PIO-USB bit-banging loop for all 3 root ports
static void core1_main() {
    sleep_ms(10);

    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = USB_PORT1_DP;
    pio_cfg.pinout = PIO_USB_PINOUT_DPDM;
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    tusb_rhport_init_t host_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(1, &host_init);


    // Register Port 2 (GP4/GP5) and Port 3 (GP6/GP7) as additional root host ports
    pio_usb_host_add_port(USB_PORT2_DP, PIO_USB_PINOUT_DPDM);
    pio_usb_host_add_port(USB_PORT3_DP, PIO_USB_PINOUT_DPDM);

    while (true) {
        tuh_task();
    }
}

// TinyUSB Host Callbacks
extern "C" void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    tuh_itf_info_t itf_info = { 0 };
    uint8_t hub_port = 1;
    tuh_daddr_info_t daddr_info = { 0 };
    if (tuh_dev_get_daddr_info(dev_addr, &daddr_info)) {
        hub_port = (daddr_info.hub_port > 0) ? daddr_info.hub_port : (daddr_info.rhport + 1);
    }

    uint16_t iface = make_interface_id(dev_addr, instance);

    static uint8_t msg_buf[7 + 512];
    uint16_t copy_len = (desc_len > 512) ? 512 : desc_len;
    memcpy(&msg_buf[0], &iface, 2);
    memcpy(&msg_buf[2], &vid, 2);
    memcpy(&msg_buf[4], &pid, 2);
    msg_buf[6] = hub_port;
    memcpy(&msg_buf[7], desc_report, copy_len);

    uart_send_frame(BridgeMsgType::DEV_CONNECTED, msg_buf, 7 + copy_len);

    tuh_hid_receive_report(dev_addr, instance);
}

extern "C" void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    uint16_t iface = make_interface_id(dev_addr, instance);
    uart_send_frame(BridgeMsgType::DEV_DISCONNECTED, (const uint8_t*)&iface, sizeof(iface));
}

extern "C" void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    uint16_t iface = make_interface_id(dev_addr, instance);

    static uint8_t msg_buf[3 + 64];
    uint16_t copy_len = (len > 64) ? 64 : len;
    memcpy(&msg_buf[0], &iface, 2);
    msg_buf[2] = 0; // TinyUSB includes report_id inside report[0] when report IDs are used
    memcpy(&msg_buf[3], report, copy_len);

    uart_send_frame(BridgeMsgType::INPUT_REPORT, msg_buf, 3 + copy_len);

    tuh_hid_receive_report(dev_addr, instance);
}

// Parse incoming OUT_REPORT / SET_FEATURE frames from nice!nano (for keyboard LEDs / rumble)
static void poll_uart_rx() {
    enum class RxState { MAGIC0, MAGIC1, TYPE, LEN0, LEN1, PAYLOAD, CRC };
    static RxState state = RxState::MAGIC0;
    static BridgeMsgType msg_type;
    static uint16_t payload_len = 0;
    static uint16_t payload_pos = 0;
    static uint8_t payload[128];

    while (uart_is_readable(BRIDGE_UART)) {
        uint8_t b = uart_getc(BRIDGE_UART);
        switch (state) {
            case RxState::MAGIC0:
                if (b == UART_BRIDGE_MAGIC0) state = RxState::MAGIC1;
                break;
            case RxState::MAGIC1:
                state = (b == UART_BRIDGE_MAGIC1) ? RxState::TYPE : RxState::MAGIC0;
                break;
            case RxState::TYPE:
                msg_type = (BridgeMsgType)b;
                state = RxState::LEN0;
                break;
            case RxState::LEN0:
                payload_len = b;
                state = RxState::LEN1;
                break;
            case RxState::LEN1:
                payload_len |= ((uint16_t)b << 8);
                payload_pos = 0;
                if (payload_len > sizeof(payload)) {
                    state = RxState::MAGIC0;
                } else {
                    state = (payload_len > 0) ? RxState::PAYLOAD : RxState::CRC;
                }
                break;
            case RxState::PAYLOAD:
                payload[payload_pos++] = b;
                if (payload_pos >= payload_len) state = RxState::CRC;
                break;
            case RxState::CRC: {
                uint8_t hdr_bytes[3] = { (uint8_t)msg_type, (uint8_t)(payload_len & 0xFF), (uint8_t)(payload_len >> 8) };
                uint8_t crc = bridge_crc8(hdr_bytes, 3);
                for (uint16_t i = 0; i < payload_len; i++) {
                    crc ^= payload[i];
                    for (uint8_t bit = 0; bit < 8; bit++) {
                        crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
                    }
                }
                if (crc == b && payload_len >= 3) {
                    uint16_t iface;
                    memcpy(&iface, &payload[0], 2);
                    uint8_t dev_addr = (iface >> 4) & 0x0F;
                    uint8_t instance = iface & 0x0F;
                    uint8_t report_id = payload[2];
                    if (msg_type == BridgeMsgType::OUT_REPORT) {
                        tuh_hid_send_report(dev_addr, instance, report_id, &payload[3], payload_len - 3);
                    } else if (msg_type == BridgeMsgType::SET_FEATURE) {
                        tuh_hid_set_report(dev_addr, instance, report_id, HID_REPORT_TYPE_FEATURE, &payload[3], payload_len - 3);
                    }
                }
                state = RxState::MAGIC0;
                break;
            }
        }
    }
}

int main() {
    // Pico-PIO-USB requires a 120 MHz (or 240 MHz) system clock
    // AK: It is set to 192 MHz in pico_sdk_init()	
    //set_sys_clock_khz(120000, true);

    uart_init(BRIDGE_UART, UART_BRIDGE_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(BRIDGE_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(BRIDGE_UART, true);

    multicore_launch_core1(core1_main);

    while (true) {
        poll_uart_rx();
        tight_loop_contents();
    }
}
