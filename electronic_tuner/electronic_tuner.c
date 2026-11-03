#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "arm_math.h"
#include "hardware/pio.h" // for mic
#include "i2s_in.pio.h"


#define MIC_SCK_PIN 26   // WS = GPIO 27
#define MIC_SD_PIN  28
#define SAMPLE_RATE 16000
#define BLOCK_SIZE  1024

#define SAMPLE_RATE 8000.0f
#define FFT_SIZE 1024
#define TEST_FREQ 440.0f

float32_t input_buf[FFT_SIZE];
float32_t output_buf[FFT_SIZE];

static PIO mic_pio = pio0;
static uint mic_sm;
static int32_t samples[BLOCK_SIZE];

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
  init_mic();

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