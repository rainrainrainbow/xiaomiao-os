/*
 * battery.c - 电池电压检测
 *
 * 分压电路: 电池正极 → 9.1k → ADC引脚 → 2.4k → GND
 * 分压比 = (9.1 + 2.4) / 2.4 = 4.792
 * ADC: 12-bit, 0-3.3V (DB_11)
 *
 * 电池电压估算:
 *   4.2V = 满电, 3.7V = 正常, 3.3V = 低电, <3.0V = 危险
 */
#include "esp_log.h"
#include "hal/keys.h"
#include "hal/battery.h"

static const char *TAG = "battery";

esp_err_t battery_init(void)
{
    ESP_LOGI(TAG, "Battery monitor: GPIO34, R1=%.1fk R2=%.1fk, ratio=%.2f",
             BATTERY_R1_KOHM, BATTERY_R2_KOHM,
             (BATTERY_R1_KOHM + BATTERY_R2_KOHM) / BATTERY_R2_KOHM);

    /* ADC 已在 keys_battery_adc_init() 中初始化 */
    return ESP_OK;
}

float battery_voltage(void)
{
    return keys_battery_voltage();
}

uint8_t battery_percentage(void)
{
    float v = battery_voltage();
    if (v <= 0) return 0;

    /* 线性映射 3.3V~4.2V → 0~100% */
    float pct = ((v - 3.3f) / (4.2f - 3.3f)) * 100.0f;
    if (pct > 100) pct = 100;
    if (pct < 0)   pct = 0;
    return (uint8_t)pct;
}

bool battery_is_low(void)
{
    return battery_voltage() < 3.3f;
}

const char *battery_status_text(void)
{
    float v = battery_voltage();
    if (v <= 0) return "未检测";
    if (v >= 4.0f) return "满电";
    if (v >= 3.7f) return "正常";
    if (v >= 3.3f) return "偏低";
    return "低电!";
}
