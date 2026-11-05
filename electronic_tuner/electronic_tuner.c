// --- Dependencies & Libraries ---
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <hardware/pio.h>
#include "pico-ssd1306/ssd1306.h"
#include "pico-ssd1306/lib/fonts/font8x8.h"
#include "i2s_in.pio.h"
#include <math.h>

#include "audio_fft.h"

int main() {
  stdio_init_all();

  init_display(SDA_PIN, SCL_PIN);
  ssd1306_clear(&display);
  ssd1306_draw_str(&display, 5, 5, "Booting!", &font8x8_font);
  ssd1306_show(&display);

  init_mic();
  sleep_ms(500);

  char fifo[24];
  sprintf(fifo, "fifo: %d", (int)pio_sm_get_rx_fifo_level(mic_pio, mic_sm));
  ssd1306_draw_str(&display, 5, 20, fifo, &font8x8_font);
  ssd1306_show(&display);
  sleep_ms(2000);

  char buf[24];
  fft_init();
  // fft_selftest();
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