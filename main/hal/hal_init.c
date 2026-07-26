/*
 * hal_init.c - 硬件一次性初始化
 */
#include "esp_log.h"
#include "hal/hal.h"

static const char *TAG = "hal";

esp_err_t hal_init_all(void)
{
    ESP_LOGI(TAG, "Initializing all hardware...");

    ESP_ERROR_CHECK(lcd_init());
    ESP_ERROR_CHECK(backlight_init());
    backlight_set(255);
    ESP_ERROR_CHECK(keys_init());
    ESP_ERROR_CHECK(i2c_bus_init());
    gyro_init();
    ESP_ERROR_CHECK(buzzer_init());
    battery_init();

    if (sdcard_init() != ESP_OK) {
        ESP_LOGW(TAG, "SD card not available");
    }

    ESP_LOGI(TAG, "HAL ready");
    return ESP_OK;
}
