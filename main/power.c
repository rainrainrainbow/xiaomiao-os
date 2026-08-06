/*
 * power.c — Battery voltage sampling, filtering, state derivation
 */
#include "power.h"
#include "driver/adc.h"
#include "esp_log.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "PWR";

/* Moving-average filter (length 8) */
#define FILTER_LEN 8
static float s_filter[FILTER_LEN] = {0};
static int   s_filter_idx = 0;
static int   s_filter_cnt = 0;

void power_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_12);
    /* Prime the filter with first reading */
    float v = power_read_voltage();
    for (int i = 0; i < FILTER_LEN; i++) s_filter[i] = v;
    s_filter_cnt = FILTER_LEN;
    ESP_LOGI(TAG, "Power init: V_bat=%.2f", v);
}

float power_read_voltage(void)
{
    /* Multi-sample (oversample x4) for noise rejection */
    uint32_t acc = 0;
    for (int i = 0; i < 4; i++) acc += adc1_get_raw(ADC1_CHANNEL_6);
    float raw = (float)(acc >> 2);
    float vadc = raw * 2.5f / 4095.0f;
    return vadc * POWER_BATTERY_DIVIDER;
}

/* Returns voltage averaged over the last FILTER_LEN samples */
static float filter_and_return(float v)
{
    s_filter[s_filter_idx] = v;
    s_filter_idx = (s_filter_idx + 1) % FILTER_LEN;
    if (s_filter_cnt < FILTER_LEN) s_filter_cnt++;

    float sum = 0;
    for (int i = 0; i < s_filter_cnt; i++) sum += s_filter[i];
    return sum / (float)s_filter_cnt;
}

float power_read_voltage_filtered(void)
{
    return filter_and_return(power_read_voltage());
}

/* Map voltage to Li-ion percentage using piecewise linear curve */
uint8_t power_read_percent(void)
{
    float v = power_read_voltage_filtered();
    /* 3.0V -> 0%, 3.7V -> 50%, 4.2V -> 100% (typical 1S Li-ion curve) */
    if (v <= 3.0f) return 0;
    if (v >= POWER_BATTERY_FULL_V) return 100;
    if (v < 3.7f) {
        /* 3.0..3.7V -> 0..50% */
        return (uint8_t)((v - 3.0f) * 50.0f / 0.7f);
    }
    /* 3.7..4.2V -> 50..100% */
    return (uint8_t)(50.0f + (v - 3.7f) * 50.0f / 0.5f);
}

power_bat_state_t power_read_state(void)
{
    float v = power_read_voltage_filtered();
    if (v < 0.5f) return POWER_BAT_UNKNOWN;     /* not yet meaningful */
    if (v <= POWER_BATTERY_CRIT_V) return POWER_BAT_CRITICAL;
    if (v <= POWER_BATTERY_LOW_V)  return POWER_BAT_LOW;
    return POWER_BAT_NORMAL;
}

void power_task(void *arg)
{
    /* Refresh filter continuously; actual UI updates triggered from main loop */
    (void)arg;
    while (true) {
        (void)power_read_voltage_filtered();  /* populate filter */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}