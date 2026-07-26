#pragma once
#include "esp_err.h"

#define BACKLIGHT_PIN  0   /* GPIO0 控制 TFT 背光 */
#define BACKLIGHT_LEDC_CH 0
#define BACKLIGHT_LEDC_TIMER LEDC_TIMER_0

esp_err_t backlight_init(void);
void      backlight_set(uint8_t brightness);  /* 0-255 */
void      backlight_off(void);
void      backlight_on(void);
uint8_t   backlight_get(void);
