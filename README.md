# Guitar Tuner — Raspberry Pi Pico 2

This is an electronic guitar tuner built on the Raspberry Pi Pico 2 (RP2350); captures audio via an I2S microphone, runs FFT-based pitch detection, and displays the detected note and tuning offset on an OLED screen.

## Overview

- Audio capture: INMP441 I2S MEMS microphone, read via the Pico's PIO-based I2S interface
- Pitch detection: FFT (CMSIS-DSP) run on windowed audio samples to identify the dominant frequency, mapped to the nearest musical note (A4 = 440 Hz reference) with cents deviation
- Display: SSD1306 128x64 OLED, driven over I2C, showing the detected note name and tuning indicator
- Platform: Raspberry Pi Pico 2 (RP2350), Pico C/C++ SDK, CMake + Ninja/MinGW Makefiles build

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

## Acknowledgements/Citations

SSD1306 driver adapted from the [pico-ssd1306](https://github.com/tapiocode/pico-ssd1306) library.