#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   // <-- add this

#ifdef __cplusplus
extern "C" {
#endif

void boa_mic_init(int bclk_pin, int ws_pin, int sd_pin, int sample_rate);
bool boa_mic_read(int16_t *buffer, size_t samples);
int  boa_mic_rms(const int16_t *buffer, size_t samples);

#ifdef __cplusplus
}
#endif

void boa_mic_beep(void);
void boa_mic_led_init(void);
void boa_mic_led_set_level(int rms);
void boa_mic_pulse_gpio(int pin);

