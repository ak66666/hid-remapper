#ifndef _UART_BRIDGE_H_
#define _UART_BRIDGE_H_

#include <stdint.h>

#define UART_BRIDGE_BAUD 921600
#define UART_BRIDGE_MAGIC0 0xAA
#define UART_BRIDGE_MAGIC1 0x55

enum class BridgeMsgType : uint8_t {
    DEV_CONNECTED    = 0x01, // [uint16_t interface, uint16_t vid, uint16_t pid, uint8_t hub_port, uint8_t desc[]]
    DEV_DISCONNECTED = 0x02, // [uint16_t interface]
    INPUT_REPORT     = 0x03, // [uint16_t interface, uint8_t report_id, uint8_t data[]]
    OUT_REPORT       = 0x04, // [uint16_t interface, uint8_t report_id, uint8_t data[]] (nRF52840 -> RP2040)
    SET_FEATURE      = 0x05, // [uint16_t interface, uint8_t report_id, uint8_t data[]] (nRF52840 -> RP2040)
};

struct __attribute__((packed)) bridge_header_t {
    uint8_t magic0;
    uint8_t magic1;
    BridgeMsgType type;
    uint16_t len; // payload length (excluding header and trailing CRC8)
};

static inline uint8_t bridge_crc8(const uint8_t* data, uint16_t len) {
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

#endif
