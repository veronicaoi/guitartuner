#include "audio_fft.h"

float fft_re[FFT_SIZE]; // real components
float fft_im[FFT_SIZE]; // imaginary components

// twiddle factor values
float tw_cos[FFT_SIZE / 2]; 
float tw_sin[FFT_SIZE / 2];
float hann[FFT_SIZE]; // hann value coefficients

ssd1306_t display;
PIO mic_pio = pio0;
uint mic_sm;
int32_t samples[BLOCK_SIZE];

void init_display(uint8_t pin_sda, uint8_t pin_scl) {
    //  init ssd1306 oled display

    i2c_init(I2C_PORT, 400000); // 400 kHz baud rate?

    //setup gpio pins
    gpio_set_function(pin_sda, GPIO_FUNC_I2C);
    gpio_set_function(pin_scl, GPIO_FUNC_I2C);
    gpio_pull_up(pin_sda);
    gpio_pull_up(pin_scl);


    ssd1306_init(&display, 128, 64, 0x3C, I2C_PORT, 0);
}

void init_mic(void) {
    // capture audio data 

    // load in i2s_in_program
    uint offset = pio_add_program(mic_pio, &i2s_in_program);


    mic_sm = pio_claim_unused_sm(mic_pio, true);
    i2s_in_program_init(mic_pio, mic_sm, offset, MIC_SCK_PIN, MIC_SD_PIN, SAMPLE_RATE);
}

void read_block(void) {
    // collect raw audio

    for (int i = 0; i < BLOCK_SIZE; i++) {
        samples[i] = (int32_t)pio_sm_get_blocking(mic_pio, mic_sm) >> 8;  // 24-bit signed...
    }

}

void fft_init(void) {
    // calculate FFT coefficients

    for (int i = 0; i < FFT_SIZE / 2; i++) {
        // divide a full rotation into increments based on window size
        float n = -2.0f * (float)M_PI * i;

        float a = n / FFT_SIZE;
        tw_cos[i] = cosf(a);
        tw_sin[i] = sinf(a);
    }

    for (int i = 0; i < FFT_SIZE; i++) {
        // Hann window: fades the edges of the block toward zero
        float angle = 2.0f * (float)M_PI * i / (FFT_SIZE - 1);

        hann[i] = 0.5f - 0.5f * cosf(angle);
    }
}

void fft(void) {
    // in-place radix-2 FFT on fft_re / fft_im

    // put the samples into bit-reversed order
    for (int i = 0; i < FFT_SIZE - 1; i++) {
        // reverse the bits of i to find the index to swap with
        int temp_i = i;
        int j = 0;

        for (int b = 1; b < FFT_SIZE; b <<= 1) {
            j = (j << 1) | (temp_i & 1);
            temp_i >>= 1;
        }

        // swap each pair only once
        if (i < j) {
            float temp = fft_re[i];
            fft_re[i] = fft_re[j];
            fft_re[j] = temp;

            temp = fft_im[i];
            fft_im[i] = fft_im[j];
            fft_im[j] = temp;
        }
    }

    // actual math part
    for (int len = 2; len <= FFT_SIZE; len <<= 1) {
        int half = len / 2;
        int step = FFT_SIZE / len;

        for (int i = 0; i < FFT_SIZE; i += len) {
            for (int k = 0; k < half; k++) {
                float wr = tw_cos[k * step];
                float wi = tw_sin[k * step];

                int a = i + k, b = a + half;

                float tr = fft_re[b] * wr - fft_im[b] * wi;
                float ti = fft_re[b] * wi + fft_im[b] * wr;

                fft_re[b] = fft_re[a] - tr;
                fft_im[b] = fft_im[a] - ti;
                fft_re[a] += tr;
                fft_im[a] += ti;
            }
        }
    }
}

static float mag2(int k) { return fft_re[k] * fft_re[k] + fft_im[k] * fft_im[k]; }

float find_peak_freq(int *bin_out) {
    // Peak frequency in Hz, parabolic-interpolated

    int peak = 1;
    
    int loop_limit = FFT_SIZE / 2 - 1;
    
    // Find the frequency bin with the maximum magnitude squared value
    for (int k = 2; k < loop_limit; k++) {
        
        if (mag2(k) > mag2(peak)) {
            peak = k;
        }
    }
    
    *bin_out = peak;
    
    // extract magnitudes of the peak and its immediate neighboring bins
    float a = sqrtf(mag2(peak - 1));
    float b = sqrtf(mag2(peak));
    float c = sqrtf(mag2(peak + 1));
    
    // Calculate the denominator for the interpolation equation
    float denom = a - (2.0f * b) + c;
    
    float d = 0.0f;
    
    // Calculate the fractional bin offset if denominator is safe from zero-division
    if (denom != 0.0f) {
        d = 0.5f * (a - c) / denom;
    }
    
    // Convert the interpolated continuous bin position into actual frequency in Hz
    float frequency = (peak + d) * (float)SAMPLE_RATE / FFT_SIZE;
    
    return frequency;
}

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
