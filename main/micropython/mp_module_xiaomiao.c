/*
 * mp_module_xiaomiao.c - xiaomiao 系统模块注册入口
 *
 * 完整实现见 micropython/port/mp_module_xiaomiao_full.c
 * 此文件在完整 MP 不可用时提供降级注册
 */
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

#include "micropython/mp_module_xiaomiao.h"

static const char *TAG = "mp_xiaomiao";

/* 完整版注册函数声明（在 mp_module_xiaomiao_full.c 中定义） */
extern void mp_register_xiaomiao_modules(void);

/* 降级版：仅打印日志 */
static void register_lite(void)
{
    ESP_LOGI(TAG, "Registering xiaomiao module (lite mode)");
    ESP_LOGI(TAG, "  xiaomiao.screen  - text/rect/clear");
    ESP_LOGI(TAG, "  xiaomiao.key     - a_pressed/b_pressed/direction");
    ESP_LOGI(TAG, "  xiaomiao.sensor  - acc_x/acc_y/battery/gyro_z");
    ESP_LOGI(TAG, "  xiaomiao.music   - tone/stop");
    ESP_LOGI(TAG, "  xiaomiao.storage - write/read/exists");
    ESP_LOGI(TAG, "  xiaomiao.system  - exit/sleep/info/reboot");
    ESP_LOGI(TAG, "  xiaomiao.time    - sleep/ticks_ms");
}

esp_err_t mp_module_xiaomiao_register(void)
{
    /* 检测是否有完整 MP */
#ifdef WITH_MICROPYTHON
    mp_register_xiaomiao_modules();
    ESP_LOGI(TAG, "xiaomiao module registered (full mode)");
#else
    register_lite();
    ESP_LOGI(TAG, "xiaomiao module registered (lite mode - MP not linked)");
#endif
    return ESP_OK;
}

/* 子模块注册（降级版占位） */
void mp_module_screen_register(void)  { ESP_LOGD(TAG, "  screen registered"); }
void mp_module_key_register(void)     { ESP_LOGD(TAG, "  key registered"); }
void mp_module_sensor_register(void)  { ESP_LOGD(TAG, "  sensor registered"); }
void mp_module_music_register(void)   { ESP_LOGD(TAG, "  music registered"); }
void mp_module_storage_register(void) { ESP_LOGD(TAG, "  storage registered"); }
void mp_module_system_register(void)  { ESP_LOGD(TAG, "  system registered"); }
void mp_module_time_register(void)   { ESP_LOGD(TAG, "  time registered"); }
