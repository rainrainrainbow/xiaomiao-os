// ================ buzzer.c - LEDC 蜂鸣器 ================

#include "buzzer.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "buzzer";

esp_err_t buzzer_init(void)
{
    ledc_timer_config_t tcfg = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 2000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&tcfg), TAG, "timer");

    ledc_channel_config_t ccfg = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ccfg), TAG, "channel");
    ESP_LOGI(TAG, "buzzer init OK (GPIO%d)", BUZZER_PIN);
    return ESP_OK;
}

static void set_freq(uint32_t freq, uint32_t duty_pct)
{
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (255 * duty_pct) / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void buzzer_beep(uint32_t freq, uint32_t duration_ms)
{
    set_freq(freq, 30);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    set_freq(freq, 0);
}

void buzzer_click(void)
{
    buzzer_beep(2000, 25);
}

void buzzer_success(void)
{
    for (int f = 1000; f <= 2000; f += 100) {
        set_freq(f, 30);
        vTaskDelay(pdMS_TO_TICKS(15));
    }
    set_freq(2000, 0);
}

void buzzer_fail(void)
{
    for (int f = 2000; f >= 500; f -= 100) {
        set_freq(f, 30);
        vTaskDelay(pdMS_TO_TICKS(15));
    }
    set_freq(500, 0);
}