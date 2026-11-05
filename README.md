# Guitar Tuner — Raspberry Pi Pico 2

This is a simple electronic guitar tuner built on the Raspberry Pi Pico 2 (RP2350); captures audio via an I2S microphone, runs FFT-based pitch detection, and displays the detected note and tuning offset on an OLED screen.

Flashing to Pico: After a successful build, do the following to run the firmware:
- hold BOOTSEL button
- plug into USB, release BOOTSEL
- drag `build/electronic_tuner.uf2` onto the RP2350 drive

## Hardware & tools

- Microcontroller: Raspberry Pi Pico 2350
- Audio input: INMP441 I2S MEMS microphone module
- Visual display: SSD2306 128x64 OLED display module
- Platforms: Pico C/C++ SDK, CMake + Ninja/MinGW Makefiles build

### Wiring
| Component | Interface | Pico Pins |
|---|---|---|
| SSD1306 OLED (128x64) | I2C1 | SDA → GPIO 18, SCL → GPIO 19, VCC → 3V3, GND → GND |
| INMP441 I2S mic | I2S (PIO) | VDD → 3V3, GND → GND, SCK → GPIO 26, WS → GPIO 27, SD → GPIO 28, L/R → GND|

## Acknowledgements & citations

SSD1306 driver adapted from the [pico-ssd1306](https://github.com/tapiocode/pico-ssd1306) library.