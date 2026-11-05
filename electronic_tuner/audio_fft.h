#ifndef AUDIO_FFT_H
#define AUDIO_FFT_H

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
#define LOWEST_BIN 4 //Ignore anything lower than this FFT bin


// --- Pin Definitions ---
#define I2C_PORT i2c1
#define SDA_PIN 18
#define SCL_PIN 19

#define MIC_SCK_PIN 26   // WS = GPIO 27
#define MIC_SD_PIN  28
#define SAMPLE_RATE 16000
#define BLOCK_SIZE  1024

#define FFT_SIZE BLOCK_SIZE   // should be a power of 2

// --- Global Variables ---
extern float fft_re[FFT_SIZE]; // real components
extern float fft_im[FFT_SIZE]; // imaginary components

// twiddle factor values
extern float tw_cos[FFT_SIZE / 2]; 
extern float tw_sin[FFT_SIZE / 2];
extern float hann[FFT_SIZE]; // hann value coefficients

extern ssd1306_t display;
extern PIO mic_pio;
extern uint mic_sm;
extern int32_t samples[BLOCK_SIZE];

// --- Function Prototypes ---
void init_display(uint8_t pin_sda, uint8_t pin_scl);
void init_mic(void);
void read_block(void);
void fft_init(void);
void fft(void);

float find_peak_freq(int *bin_out);
void fft_selftest(void);
float analyze_block(void);

float note_number_exact(float frequency);
int frequency_to_note(float frequency);
float cents_from_note(float frequency, int note);
void note_name(int note, char *name);
#endif // AUDIO_FFT_H
