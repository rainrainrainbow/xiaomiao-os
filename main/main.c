/*
 * main.c - XiaoMiao OS v1.0 桌面系统入口
 *
 * 启动流程:
 *   1. return_to_loader_setup() - 确保 Reset 回 Loader
 *   2. NVS 初始化
 *   3. HAL 硬件初始化 (LCD/SD/Keys/I2C/ADC/Buzzer/Gyro)
 *   4. UI 框架 (LVGL 9.5 + 主题 + 页面管理)
 *   5. 状态栏 / MicroPython / App 管理器
 *   6. 桌面创建
 *   7. 清除 Loader 魔数
 *   8. 启动按键扫描任务 + UI 主任务
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"

#include "return_to_loader.h"

/* LVGL 必须在 HAL 之前引入（HAL 的 lcd.c 用到 lv_ 类型） */
#include "lvgl.h"

/* HAL */
#include "hal/lcd.h"
#include "hal/sdcard.h"
#include "hal/keys.h"
#include "hal/backlight.h"
#include "hal/battery.h"
#include "hal/i2c_bus.h"
#include "hal/gyro.h"
#include "hal/buzzer.h"

/* UI */
#include "ui/theme.h"
#include "ui/page_manager.h"

/* Desktop */
#include "desktop/desktop.h"
#include "desktop/statusbar.h"
#include "desktop/settings.h"
#include "desktop/launcher.h"

/* Block Editor */
#include "block_editor/editor.h"

/* MicroPython */
#include "micropython/mp_engine.h"

/* App Runtime */
#include "app_runtime/app_manager.h"

static const char *TAG = "xiaomiao-os";

/* 按键事件队列 */
static QueueHandle_t s_key_queue = NULL;
#define KEY_QUEUE_SIZE 16

/* 按键扫描任务 (10ms 周期) */
static void key_scan_task(void *param)
{
    ESP_LOGI(TAG, "Key scan task started");
    while (1) {
        keys_update();

        key_code_t k = keys_scan();
        if (k != KEY_NONE) {
            xQueueSend(s_key_queue, &k, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* UI 主任务 + 按键分发 */
static void ui_main_task(void *param)
{
    ESP_LOGI(TAG, "UI main task started");

    uint32_t last_tick = 0;

    while (1) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        /* LVGL 心跳 */
        if (now - last_tick >= 5) {
            lv_timer_handler();
            last_tick = now;
        }

        /* 状态栏每秒更新 */
        static uint32_t last_status = 0;
        if (now - last_status >= 1000) {
            statusbar_update();
            last_status = now;
        }

        /* 按键分发 */
        key_code_t key;
        if (xQueueReceive(s_key_queue, &key, 0) == pdTRUE) {
            ESP_LOGD(TAG, "Key: %d", key);

            lv_obj_t *desk    = page_get(PAGE_DESKTOP);
            lv_obj_t *editor  = page_get(PAGE_BLOCK_EDITOR);
            lv_obj_t *settings= page_get(PAGE_SETTINGS);
            lv_obj_t *applist = page_get(PAGE_APP_LIST);

            bool e = editor   && !lv_obj_has_flag(editor,   LV_OBJ_FLAG_HIDDEN);
            bool s = settings && !lv_obj_has_flag(settings, LV_OBJ_FLAG_HIDDEN);
            bool a = applist  && !lv_obj_has_flag(applist,  LV_OBJ_FLAG_HIDDEN);
            bool d = desk     && !lv_obj_has_flag(desk,     LV_OBJ_FLAG_HIDDEN);

            if (e)       block_editor_handle_key(key);
            else if (s)   settings_handle_key(key);
            else if (a)   launcher_handle_key(key);
            else if (d)   desktop_handle_key(key);
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void app_main(void)
{
    /* 1. Loader 返回机制 */
    return_to_loader_setup();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  XiaoMiao OS v1.0.0");
    ESP_LOGI(TAG, "  ESP32-WROVER-B @ 240MHz");
    ESP_LOGI(TAG, "  Flash: 4MB  PSRAM: 8MB");
    ESP_LOGI(TAG, "========================================");

    /* 2. NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* 3. HAL */
    ESP_LOGI(TAG, "[HAL] Init...");
    ESP_ERROR_CHECK(lcd_init());
    ESP_ERROR_CHECK(backlight_init());
    backlight_set(255);
    ESP_ERROR_CHECK(keys_init());
    ESP_ERROR_CHECK(i2c_bus_init());
    gyro_init();
    ESP_ERROR_CHECK(buzzer_init());
    battery_init();
    if (sdcard_init() != ESP_OK) {
        ESP_LOGW(TAG, "[HAL] SD not mounted - App features limited");
    }

    /* 4. UI */
    ESP_LOGI(TAG, "[UI] Init LVGL 9.5...");
    ui_theme_init();
    page_manager_init();

    /* 5. 状态栏 */
    statusbar_create();

    /* 6. MicroPython */
    ESP_LOGI(TAG, "[MP] Init...");
    mp_engine_init();

    /* 7. App 管理器 */
    ESP_LOGI(TAG, "[APP] Scan...");
    app_manager_init();

    /* 8. 桌面 */
    ESP_LOGI(TAG, "[DESK] Create...");
    desktop_create();

    /* 9. 清除 Loader 魔数 */
    return_to_loader_clear();

    /* 10. 启动任务 */
    s_key_queue = xQueueCreate(KEY_QUEUE_SIZE, sizeof(key_code_t));
    xTaskCreatePinnedToCore(key_scan_task, "key_scan", 2048,  NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(ui_main_task,  "ui_main",  8192,  NULL, 4, NULL, 1);

    ESP_LOGI(TAG, "System ready!");
}
