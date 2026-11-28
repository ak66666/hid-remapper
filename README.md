# HID Remapper

<< THE ORIGINAL PROJECT DESCRIPTION IS AFTER THE CUSTOM ADDITION SECTION >>

## Custom Additions (`password_protection` branch)
<< 

This fork extends the `nice!nano` (nRF52840) Bluetooth build and web configurator with three additional features:

### 1. Optional Configuration Password Protection
- Protects device configuration writes (`Save to device`), Bluetooth pairing/unpairing, and firmware flashing behind an optional user-defined password stored in non-volatile flash (`remapper/auth`).
- **Read-Only Access While Locked:** Loading existing mappings (`Load from device`), monitoring live HID inputs, and viewing paired Bluetooth devices remain accessible without unlocking.
- **Session Unlock:** Entering the password in the web configurator unlocks write operations for the current USB connection; unplugging or resetting the device automatically re-locks it.
- Supports setting, changing, and removing the password directly from the **Actions** tab in the web configurator.

### 2. Paired Bluetooth Devices Manager
- Displays all bonded Bluetooth LE HID devices (`index`, `MAC address`, `address type`, and live `Connected` / `Disconnected` status) in the **Actions** tab.
- Automatically refreshes when devices pair, connect, disconnect, or are forgotten, as well as when the web configurator is opened or the remapper is plugged in.
- Supports forgetting individual paired devices (`Forget`) or clearing all bonds (`Forget all devices`).

### 3. Wired USB Ports via Pro Micro RP2040 UART Co-Processor
Adds **3 wired USB-A host ports** (`Hub port` 1, 2, and 3) to the `nice!nano` Bluetooth build using a **Pro Micro RP2040** running `remapper_uart_bridge.uf2` over a `921,600` baud UART link (`UART0`), allowing wired USB and Bluetooth devices to share a single configuration, layer state, expressions, and password lock.

#### Microcontroller Wiring (`nice!nano` <-> `Pro Micro RP2040`)

| `nice!nano` Pin | `Pro Micro RP2040` Pin | Function |
| :--- | :--- | :--- |
| `VBUS` / `RAW` (5V) | `RAW` (5V) | 5V supply from USB-C *(tap pre-diode `VBUS` if `RAW` drops below `4.6V`)* |
| `GND` | `GND` | Common ground |
| `TX` (`P0.06`) | `RX` (`GP1`) | UART0 (`nice!nano` -> `RP2040` for LED / `OUT` & `FEATURE` reports) |
| `RX` (`P0.08`) | `TX` (`GP0`) | UART0 (`RP2040` -> `nice!nano` for HID descriptors & input reports) |

#### Hand-Wired USB-A Sockets (`Pro Micro RP2040`)
Each USB-A female socket requires a **`15 KOhm` pull-down resistor** (`10 KOhm`–`22 KOhm`) from both `D+` and `D-` to `GND` (6 resistors total):

| USB-A Socket | Pin 1 (`5V`) | Pin 2 (`D-`) | Pin 3 (`D+`) | Pin 4 (`GND`) | Pull-Down Resistors |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Socket 1** (`Hub port 1`) | `5V` (`VBUS`) | `GP3` | `GP2` | `GND` | `15 KOhm` on `GP2` & `GP3` to `GND` |
| **Socket 2** (`Hub port 2`) | `5V` (`VBUS`) | `GP5` | `GP4` | `GND` | `15 KOhm` on `GP4` & `GP5` to `GND` |
| **Socket 3** (`Hub port 3`) | `5V` (`VBUS`) | `GP7` | `GP6` | `GND` | `15 KOhm` on `GP6` & `GP7` to `GND` |

- **Status LED (`GP17`):** Blinks 3 times on boot, pulses `150 ms` when a wired USB device connects or disconnects, and pulses `20 ms` on HID report traffic.

### Building & Flashing
Pushing to `password_protection` automatically builds both firmware images via GitHub Actions (`.github/workflows/build-nice-nano.yml`) and deploys the web configurator to GitHub Pages:
1. **`remapper_nice_nano.uf2`:** Double-tap reset on the `nice!nano` and copy onto the `NICENANO` drive.
2. **`remapper_uart_bridge.uf2`:** Enter bootloader mode on the `Pro Micro RP2040` and copy onto the `RPI-RP2` drive.


<< BELOW IS THE ORIGINAL PROJECT DESCRIPTION >>





# HID Remapper

_For user documentation please see the project's website at [remapper.org](https://www.remapper.org/)._

This is a configurable USB dongle that allows you to remap inputs from mice, keyboards and other devices. It works completely in hardware and requires no software running on the computer during normal use.

It can do things like reassign buttons, change keyboard layouts, map mouse buttons to keyboard inputs, map keystrokes to mouse inputs, change mouse sensitivity (permanently or when a button is held), rotate mouse axes by arbitrary (non-90 degree) angles, drag-lock for mouse buttons, scroll by moving the mouse, and much more.

It is configurable [through a web browser](https://www.remapper.org/config/) using WebHID (Chrome or Chrome-based browser required).

Wireless receivers are supported and multiple devices can be connected at the same time using a USB hub (with different mappings for each device if desired).

In addition to the remapping functionality, it can do polling rate overclocking up to 1000 Hz.

A separate [serial](SERIAL.md) version of the remapper takes inputs from a serial (RS-232) mouse and translates them to USB.

There are also [Bluetooth](BLUETOOTH.md) versions that run on either the Pico W / Pico 2W or nRF52840-based boards, which translate Bluetooth inputs to USB.

![HID Remapper](images/remapper1.jpg)

## How to make the device

There are three main ways of making the HID Remapper. You can either buy [this board](https://www.adafruit.com/product/5723) from Adafruit, make it yourself using a Raspberry Pi Pico (or two), or you can use the provided files to manufacture a custom board at JLCPCB or a similar service. The functionality is the same in all cases.

If you get the Feather RP2040 USB Host board from Adafruit, the device is ready to use, you just need to flash it with the right firmware ([remapper\_feather.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_feather.uf2)). Hold the "Boot" button on the board, then press the "Reset" button. A USB drive should show up on your computer. Copy the UF2 file to that drive. That's it.

See [here](HARDWARE.md) for details on how to make the Pico variants of the device and [here](custom-boards/) for details on the custom board option.

## How to use the configuration tool

A live version of the web configuration tool can be found at [remapper.org/config](https://www.remapper.org/config/).

For details on how to use it, please see the [HID Remapper Manual](https://www.remapper.org/manual/).

If you can't use the browser-based configuration tool, there's also a [command-line tool](config-tool) that takes JSON in the same format as the web tool on standard input. I only tested it on Linux, but in theory it should also run on Windows and Mac.

## How to update the firmware

The procedure to update the firmware is similar on all variants. When you go to the configuration website and try to connect to your device when it doesn't have the latest firmware, you will get a message and a link to a version of the configuration interface that is compatible with your current (old) firmware. Click that link, connect to your HID Remapper by clicking "Open device" as usual, then go to the "Actions" tab and click "Flash firmware". This will put your device in firmware flashing mode. A drive should appear on your computer. For all the RP2040-based variants, the drive will be named "RPI-RP2". For the Bluetooth variants, it will be called something else, depending on what board you're using. Download the correct firmware file for your variant (see table below) and copy it to that drive. On custom boards v1, v2, v5, v6 and v7 (dual RP2040 boards), after flashing the firmware you have to disconnect and reconnect your HID Remapper. That's it, you can go back to the regular version of the configuration interface and carry on.

_(Please note that previously a manual "Flash B side" step was required on custom boards v1, v2, v5, v6 and v7. That is no longer necessary.)_

If you're using the dual Pico variant then you need to flash the A side using the [remapper\_dual\_a.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_dual_a.uf2) file as described above and then flash the B side manually. Disconnect your HID Remapper from your computer, disconnect the OTG adapter from the B-side Pico, hold the BOOTSEL button on the B-side Pico and then, while holding the button, connect the B-side Pico to your computer. A drive named "RPI-RP2" should appear. Copy the [remapper\_dual\_b.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_dual_b.uf2) file to that drive. Disconnect the B-side Pico from your computer, reconnect the OTG adapter and reconnect your HID Remapper to your computer.

When updating firmware, the current configuration on your HID Remapper is preserved. For extra peace of mind you can export your configuration to a JSON file before performing the update. That way if you need to revert to the old version of the firmware for any reason, you'll be able to import the configuration from the JSON file (configuration is lost when going from a newer firmware to an older firmware).

variant | firmware file(s) | notes
------- | ---------------- | -----------------------
single Pico | [remapper.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper.uf2) |
dual Pico | [remapper\_dual\_a.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_dual_a.uf2)<br>[remapper\_dual\_b.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_dual_b.uf2) | each Pico needs to be flashed separately
Feather RP2040 with USB Host | [remapper\_feather.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_feather.uf2) |
custom board v1 | [remapper\_board.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board.uf2) | disconnect and reconnect after flashing
custom board v2 | [remapper\_board.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board.uf2) | disconnect and reconnect after flashing
custom board v3 | [remapper\_feather.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_feather.uf2) |
custom board v4 | [remapper\_feather.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_feather.uf2) |
custom board v5 | [remapper\_board.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board.uf2) | disconnect and reconnect after flashing
custom board v6 | [remapper\_board.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board.uf2) | disconnect and reconnect after flashing
custom board v7 | [remapper\_board\_v7.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board_v7.uf2) | disconnect and reconnect after flashing
custom board v8 | [remapper\_board\_v8.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board_v8.uf2) |
Pico W | [remapper_bt.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_bt.uf2) |
Pico 2W | [remapper_bt_pico2_w.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_bt_pico2_w.uf2) |
Feather nRF52840 Express | [remapper_adafruit_feather_nrf52840.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_adafruit_feather_nrf52840.uf2) |
Xiao nRF52840 | [remapper_seeed_xiao_nrf52840.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_seeed_xiao_nrf52840.uf2) |
nice!nano | [remapper_nice_nano.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_nice_nano.uf2) |
serial | [remapper_serial.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_serial.uf2) |

For boards not listed above, use the same file name you used when flashing it for the first time.

## How to compile the firmware

The easiest way to compile the firmware is to let GitHub do it for you. This repository has GitHub Actions that build the firmware, so you can just fork, make your changes, wait for the job to complete, and look for the binaries in the artifacts produced.

To compile the RP2040 firmware on your machine, use the following steps (details may vary depending on your Linux distribution):

```
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib srecord
git clone https://github.com/jfedor2/hid-remapper.git
cd hid-remapper
git submodule update --init
cd firmware
mkdir build
cd build
cmake ..
# or, to build for the custom boards:
# PICO_BOARD=remapper cmake ..
make
```

To compile the nRF52 firmware, run `firmware-bluetooth/build.sh`. It checks out the nRF Connect SDK under `firmware-bluetooth/sdk` and builds all the supported boards, putting the results in `firmware-bluetooth/artifacts`. It needs west and the Zephyr SDK, so the easiest way is to run it in Docker like this (start from the top level of the repository or adjust the path accordingly):

```
docker run --rm -v $(pwd):/workdir/project -w /workdir/project/firmware-bluetooth ghcr.io/zephyrproject-rtos/ci:v0.29.2 ./build.sh
```

You can pass build names (like `seeed_xiao_nrf52840`) to `build.sh` to only build some of the boards.

## License

The software in this repository is licensed under the [MIT License](LICENSE), unless stated otherwise.

The hardware designs in this repository are licensed under the Creative Commons Attribution 4.0 International license ([CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)), unless stated otherwise.

 