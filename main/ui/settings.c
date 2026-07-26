/*
 * settings.c - 全局设置 (NVS 持久化)
 *
 * 保存: 亮度/音量/壁纸/语言/已安装App列表/桌面排序
 */
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "ui/settings.h"
#include "ui/system.h"

static const char *TAG = "settings";

/* NVS 命名空间 */
#define NVS_NS "xm_settings"

/* 默认设置 */
static system_settings_t s_settings = {
    .brightness = 255,
    .volume = 80,
    .language = 0,        /* 0=中文 */
    .wifi_ssid = "",
    .wifi_password = "",
    .auto_sleep = 60,
    .debug_mode = false,
};

/* ---------- API ---------- */
void settings_load(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "No saved settings, using defaults");
        return;
    }

    int32_t v;
    size_t len;

    if (nvs_get_i32(h, "brightness", &v) == ESP_OK) s_settings.brightness = (uint8_t)v;
    if (nvs_get_i32(h, "volume", &v) == ESP_OK)     s_settings.volume = (uint8_t)v;
    if (nvs_get_i32(h, "language", &v) == ESP_OK)    s_settings.language = (uint8_t)v;
    if (nvs_get_i32(h, "auto_sleep", &v) == ESP_OK)  s_settings.auto_sleep = (uint16_t)v;
    if (nvs_get_i32(h, "debug", &v) == ESP_OK)       s_settings.debug_mode = (bool)v;

    len = sizeof(s_settings.wifi_ssid);
    nvs_get_str(h, "wifi_ssid", s_settings.wifi_ssid, &len);
    len = sizeof(s_settings.wifi_password);
    nvs_get_str(h, "wifi_pwd", s_settings.wifi_password, &len);

    nvs_close(h);
    ESP_LOGI(TAG, "Settings loaded: brightness=%d volume=%d lang=%d",
             s_settings.brightness, s_settings.volume, s_settings.language);
}

void settings_save(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for write");
        return;
    }

    nvs_set_i32(h, "brightness", s_settings.brightness);
    nvs_set_i32(h, "volume", s_settings.volume);
    nvs_set_i32(h, "language", s_settings.language);
    nvs_set_i32(h, "auto_sleep", s_settings.auto_sleep);
    nvs_set_i32(h, "debug", s_settings.debug_mode);
    nvs_set_str(h, "wifi_ssid", s_settings.wifi_ssid);
    nvs_set_str(h, "wifi_pwd", s_settings.wifi_password);

    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Settings saved");
}

system_settings_t *settings_get(void) { return &s_settings; }

void settings_set_brightness(uint8_t val) { s_settings.brightness = val; settings_save(); }
void settings_set_volume(uint8_t val)     { s_settings.volume = val; settings_save(); }
void settings_set_language(uint8_t lang)  { s_settings.language = lang; settings_save(); }
void settings_set_wifi(const char *ssid, const char *pwd)
{
    strncpy(s_settings.wifi_ssid, ssid, sizeof(s_settings.wifi_ssid)-1);
    strncpy(s_settings.wifi_password, pwd, sizeof(s_settings.wifi_password)-1);
    settings_save();
}
