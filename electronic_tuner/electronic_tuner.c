#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <hardware/pio.h>
#include "pico-ssd1306/ssd1306.h"
#include "pico-ssd1306/lib/fonts/font8x8.h"
#include "i2s_in.pio.h"
#include <math.h>

#define I2C_PORT i2c1
#define SDA_PIN 18
#define SCL_PIN 19

#define MIC_SCK_PIN 26   // WS = GPIO 27
#define MIC_SD_PIN  28
#define SAMPLE_RATE 16000
#define BLOCK_SIZE  1024

#define FFT_SIZE BLOCK_SIZE   // should be a power of 2

static float fft_re[FFT_SIZE], fft_im[FFT_SIZE];
static float tw_cos[FFT_SIZE / 2], tw_sin[FFT_SIZE / 2];
static float hann[FFT_SIZE];

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

void fft_init(void) {
  for (int i = 0; i < FFT_SIZE / 2; i++) {
    float a = -2.0f * (float)M_PI * i / FFT_SIZE;
    tw_cos[i] = cosf(a);
    tw_sin[i] = sinf(a);
  }
  for (int i = 0; i < FFT_SIZE; i++) {
    hann[i] = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * i / (FFT_SIZE - 1));
  }
}

// In-place radix-2 FFT on fft_re / fft_im
void fft(void) {
  unsigned j = 0;
  for (unsigned i = 0; i < FFT_SIZE - 1; i++) {
    if (i < j) {
      float t = fft_re[i]; fft_re[i] = fft_re[j]; fft_re[j] = t;
      t = fft_im[i]; fft_im[i] = fft_im[j]; fft_im[j] = t;
    }
    unsigned m = FFT_SIZE >> 1;
    while (m >= 1 && (j & m)) { j ^= m; m >>= 1; }
    j |= m;
  }
  for (unsigned len = 2; len <= FFT_SIZE; len <<= 1) {
    unsigned half = len >> 1, step = FFT_SIZE / len;
    for (unsigned i = 0; i < FFT_SIZE; i += len) {
      for (unsigned k = 0; k < half; k++) {
        float wr = tw_cos[k * step], wi = tw_sin[k * step];
        unsigned a = i + k, b = a + half;
        float tr = fft_re[b] * wr - fft_im[b] * wi;
        float ti = fft_re[b] * wi + fft_im[b] * wr;
        fft_re[b] = fft_re[a] - tr;  fft_im[b] = fft_im[a] - ti;
        fft_re[a] += tr;             fft_im[a] += ti;
      }
    }
  }
}

static float mag2(int k) { return fft_re[k] * fft_re[k] + fft_im[k] * fft_im[k]; }

// Peak frequency in Hz, parabolic-interpolated; raw bin returned via *bin_out
float find_peak_freq(int *bin_out) {
  int peak = 1;
  for (int k = 2; k < FFT_SIZE / 2 - 1; k++)
    if (mag2(k) > mag2(peak)) peak = k;
  *bin_out = peak;
  float a = sqrtf(mag2(peak - 1)), b = sqrtf(mag2(peak)), c = sqrtf(mag2(peak + 1));
  float denom = a - 2.0f * b + c;
  float delta = (denom != 0.0f) ? 0.5f * (a - c) / denom : 0.0f;
  return (peak + delta) * (float)SAMPLE_RATE / FFT_SIZE;
}

// Phase 5 check: known sine in, expected frequency out
void fft_selftest(void) {
  const float tests[] = {82.41f, 329.63f, 440.0f};
  char line[24];
  ssd1306_clear(&display);
  for (int t = 0; t < 3; t++) {
    for (int i = 0; i < FFT_SIZE; i++) {
      fft_re[i] = sinf(2.0f * (float)M_PI * tests[t] * i / SAMPLE_RATE) * hann[i];
      fft_im[i] = 0.0f;
    }
    fft();
    int bin;
    float f = find_peak_freq(&bin);
    printf("in %.2f Hz -> bin %d, peak %.2f Hz\n", tests[t], bin, f);
    sprintf(line, "%.0f > %.1f", tests[t], f);
    ssd1306_draw_str(&display, 5, 5 + t * 20, line, &font8x8_font);
  }
  ssd1306_show(&display);
  sleep_ms(4000);
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
  fft_init();
  fft_selftest();
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