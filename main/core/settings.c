// ================ settings.c - NVS 设置 ================

#include "settings.h"
#include "hal/battery.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "settings";

static struct {
    bool wifi_on;
    int  volume;        // 0~120
    int  brightness;    // 0~120 (UI 占位, 背光硬件硬接)
    bool has_user_app;
} s = { true, 60, 80, false };

esp_err_t settings_init(void)
{
    nvs_handle_t h;
    if (nvs_open("os_settings", NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGW(TAG, "nvs open fail, use defaults");
        return ESP_OK;
    }
    uint8_t v;
    size_t sz = 1;
    if (nvs_get_blob(h, "wifi", &v, &sz) == ESP_OK) s.wifi_on = v;
    sz = sizeof(int);
    nvs_get_blob(h, "vol", &s.volume, &sz);
    nvs_get_blob(h, "bri", &s.brightness, &sz);
    sz = 1;
    if (nvs_get_blob(h, "user_app", &v, &sz) == ESP_OK) s.has_user_app = v;
    nvs_close(h);
    ESP_LOGI(TAG, "settings loaded wifi=%d vol=%d bri=%d", s.wifi_on, s.volume, s.brightness);
    return ESP_OK;
}

bool settings_wifi_on(void) { return s.wifi_on; }
int  settings_volume(void)  { return s.volume; }
float settings_battery_v(void) { return battery_voltage(); }
void settings_note_user_app(void)
{
    s.has_user_app = true;
    nvs_handle_t h;
    if (nvs_open("os_settings", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_blob(h, "user_app", (uint8_t[1]){1}, 1);
        nvs_commit(h);
        nvs_close(h);
    }
}

void settings_advance(int idx)
{
    switch (idx) {
        case 0:  // Wi-Fi toggle
            s.wifi_on = !s.wifi_on;
            break;
        case 1:  // 音量 0→20→40→60→80→100→0
            s.volume = (s.volume + 20) % 120;
            if (s.volume == 0) s.volume = 20;
            break;
        case 2:  // 亮度
            s.brightness = (s.brightness + 20) % 120;
            if (s.brightness == 0) s.brightness = 20;
            break;
        default: break;
    }
    // 持久化
    nvs_handle_t h;
    if (nvs_open("os_settings", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, "wifi", (uint8_t[1]){s.wifi_on ? 1 : 0}, 1);
    nvs_set_blob(h, "vol", &s.volume, sizeof(int));
    nvs_set_blob(h, "bri", &s.brightness, sizeof(int));
    nvs_commit(h);
    nvs_close(h);
}