/*
 * backlight.c - GPIO0 PWM 背光控制 (LEDC)
 */
#include "esp_log.h"
#include "driver/ledc.h"
#include "hal/backlight.h"

static const char *TAG = "backlight";
static uint8_t s_brightness = 255;

esp_err_t backlight_init(void)
{
    ESP_LOGI(TAG, "Initializing backlight on GPIO%d (LEDC)", BACKLIGHT_PIN);

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = BACKLIGHT_LEDC_TIMER,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num = BACKLIGHT_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BACKLIGHT_LEDC_CH,
        .timer_sel = BACKLIGHT_LEDC_TIMER,
        .duty = 255,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    return ESP_OK;
}

void backlight_set(uint8_t brightness)
{
    s_brightness = brightness;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_LEDC_CH, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_LEDC_CH);
}

void backlight_off(void) { backlight_set(0); }
void backlight_on(void)  { backlight_set(255); }
uint8_t backlight_get(void) { return s_brightness; }
