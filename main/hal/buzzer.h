#pragma once
#include "esp_err.h"

#define BUZZER_PIN      14
#define BUZZER_LEDC_CH  1
#define BUZZER_LEDC_TIMER LEDC_TIMER_1

esp_err_t buzzer_init(void);
void      buzzer_tone(uint16_t freq_hz, uint16_t duration_ms); /* 0=duration 表示持续 */
void      buzzer_stop(void);
void      buzzer_melody(const uint16_t *freqs, const uint16_t *durations, uint8_t count);
