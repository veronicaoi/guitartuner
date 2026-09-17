# Guitar Tuner — Raspberry Pi Pico 2

A real-time electronic guitar tuner built on the Raspberry Pi Pico 2 (RP2350). Captures audio via an I2S microphone, runs FFT-based pitch detection, and displays the detected note and tuning offset on an OLED screen.

## Overview

- **Audio capture:** INMP441 I2S MEMS microphone, read via the Pico's PIO-based I2S interface
- **Pitch detection:** FFT (CMSIS-DSP) run on windowed audio samples to identify the dominant frequency, mapped to the nearest musical note (A4 = 440 Hz reference) with cents deviation
- **Display:** SSD1306 128x64 OLED, driven over I2C, showing the detected note name and tuning indicator
- **Platform:** Raspberry Pi Pico 2 (RP2350), Pico C/C++ SDK, CMake + Ninja/MinGW Makefiles build

## Hardware

| Component | Interface | Pico Pins |
|---|---|---|
| SSD1306 OLED (128x64) | I2C1 | SDA → GPIO 18, SCL → GPIO 19, VCC → 3V3, GND → GND |
| INMP441 I2S mic | I2S (PIO) | *TBD — wiring in progress* |

## Project Status

- [x] Toolchain set up (CMake, Ninja/MinGW, ARM GCC, Pico SDK 2.2.0)
- [x] Basic build/flash pipeline confirmed working
- [x] SSD1306 display wired and driven over I2C — static and live-updating text confirmed working
- [ ] INMP441 microphone wired and I2S PIO audio capture
- [ ] FFT pitch detection (CMSIS-DSP), validated against a known sine wave
- [ ] Note/cents mapping and tuning display
- [ ] Final integration: live tuner display on OLED

## Building

Requires:
- [CMake](https://cmake.org/download/)
- [Ninja](https://github.com/ninja-build/ninja/releases) or MinGW (depending on configured generator)
- [Arm GNU Toolchain (arm-none-eabi)](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (set `PICO_SDK_PATH` to its location)

```powershell
mkdir build
cd build
cmake ..
cmake --build .
```

This produces `electronic_tuner.uf2` in the `build` folder.

## Flashing

1. Hold **BOOTSEL** on the Pico, plug it into USB, then release BOOTSEL
2. Drag `build/electronic_tuner.uf2` onto the `RPI-RP2` drive that appears
3. The board reboots automatically and runs the new firmware

## Acknowledgements

SSD1306 driver adapted from the [pico-ssd1306](https://github.com/tapiocode/pico-ssd1306) library.