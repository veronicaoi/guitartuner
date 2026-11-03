#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <hardware/pio.h>
#include "pico-ssd1306/ssd1306.h"
#include "pico-ssd1306/lib/fonts/font8x8.h"
#include "i2s_in.pio.h"

#define I2C_PORT i2c1
#define SDA_PIN 18
#define SCL_PIN 19

#define MIC_SCK_PIN 26   // WS = GPIO 27
#define MIC_SD_PIN  28
#define SAMPLE_RATE 16000
#define BLOCK_SIZE  1024

static ssd1306_t display;
static PIO mic_pio = pio0;
static uint mic_sm;
static int32_t samples[BLOCK_SIZE];

void init_display(uint8_t pin_sda, uint8_t pin_scl) {
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(pin_sda, GPIO_FUNC_I2C);
  gpio_set_function(pin_scl, GPIO_FUNC_I2C);
  gpio_pull_up(pin_sda);
  gpio_pull_up(pin_scl);
  ssd1306_init(&display, 128, 64, 0x3C, I2C_PORT, 0);
}

void init_mic(void) {
  uint offset = pio_add_program(mic_pio, &i2s_in_program);
  mic_sm = pio_claim_unused_sm(mic_pio, true);
  i2s_in_program_init(mic_pio, mic_sm, offset, MIC_SCK_PIN, MIC_SD_PIN, SAMPLE_RATE);
}

void read_block(void) {
  for (int i = 0; i < BLOCK_SIZE; i++) {
    samples[i] = (int32_t)pio_sm_get_blocking(mic_pio, mic_sm) >> 8;  // 24-bit signed
  }
}

int main() {
  stdio_init_all();

  init_display(SDA_PIN, SCL_PIN);
  ssd1306_clear(&display);
  ssd1306_draw_str(&display, 5, 5, "booting", &font8x8_font);
  ssd1306_show(&display);

  init_mic();
  sleep_ms(500);

  char fifo[24];
  sprintf(fifo, "fifo: %d", (int)pio_sm_get_rx_fifo_level(mic_pio, mic_sm));
  ssd1306_draw_str(&display, 5, 20, fifo, &font8x8_font);
  ssd1306_show(&display);
  sleep_ms(2000);

  char buf[24];
  while (1) {
    read_block();

    int32_t min = samples[0], max = samples[0];
    for (int i = 1; i < BLOCK_SIZE; i++) {
      if (samples[i] < min) min = samples[i];
      if (samples[i] > max) max = samples[i];
    }
    int32_t p2p = max - min;
    printf("min %ld max %ld p2p %ld\n", (long)min, (long)max, (long)p2p);

    int w = p2p / 16384;
    if (w > display.width) w = display.width;

    ssd1306_clear(&display);
    sprintf(buf, "p2p: %ld", (long)p2p);
    ssd1306_draw_str(&display, 5, 10, buf, &font8x8_font);
    ssd1306_fill_rect(&display, 0, 40, w, 10);
    ssd1306_show(&display);
  }
}