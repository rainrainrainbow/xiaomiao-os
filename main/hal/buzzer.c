/*
 * buzzer.c - 蜂鸣器驱动 (LEDC Timer1/Ch1, GPIO14)
 */
#include "esp_log.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hal/buzzer.h"

static const char *TAG = "buzzer";
static bool s_playing = false;

esp_err_t buzzer_init(void)
{
    ESP_LOGI(TAG, "Initializing buzzer on GPIO%d (LEDC)", BUZZER_PIN);

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = BUZZER_LEDC_TIMER,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BUZZER_LEDC_CH,
        .timer_sel = BUZZER_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    return ESP_OK;
}

void buzzer_tone(uint16_t freq_hz, uint16_t duration_ms)
{
    if (freq_hz == 0) { buzzer_stop(); return; }

    ledc_set_freq(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_TIMER, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CH, 512); /* 50% */
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CH);

    s_playing = true;

    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        buzzer_stop();
    }
}

void buzzer_stop(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CH, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CH);
    s_playing = false;
}

void buzzer_melody(const uint16_t *freqs, const uint16_t *durations, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        if (freqs[i] == 0) {
            buzzer_stop();
        } else {
            ledc_set_freq(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_TIMER, freqs[i]);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CH, 512);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CH);
            s_playing = true;
        }
        vTaskDelay(pdMS_TO_TICKS(durations[i]));
    }
    buzzer_stop();
}
