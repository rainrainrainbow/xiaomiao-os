// ================ main.c - 小喵 OS 入口 ================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "lvgl.h"

#include "hal/lcd_st7735.h"
#include "hal/keys.h"
#include "hal/sdcard.h"
#include "hal/buzzer.h"
#include "hal/battery.h"
#include "hal/mpu6050.h"
#include "hal/led_motor.h"
#include "ui/theme.h"
#include "ui/input.h"
#include "ui/toast.h"
#include "ui/canvas.h"
#include "core/settings.h"
#include "core/app_manager.h"
#include "core/app_loader.h"
#include "mpy/mp_runtime.h"
#include "mpy/mp_bindings.h"
#include "return_to_loader.h"

static const char *TAG = "main";

// ================ LVGL flush 回调（三缓冲） ================
static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t *s_buf1, *s_buf2;

static void lcd_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    (void)area;
    lcd_flush((uint16_t *)color_p, LV_HOR_RES * LV_VER_RES);
    lv_disp_flush_ready(drv);
}

// ================ 状态栏刷新任务 ================
static void statusbar_task(void *arg)
{
    while (1) {
        // 状态栏更新由 app_manager 内部接管
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ================ 按键 → app_manager 派发 ================
static void key_dispatch_task(void *arg)
{
    while (1) {
        key_event_t e;
        if (keys_pop(&e)) {
            if (!e.long_press) {
                switch (e.code) {
                    case KEY_UP:    app_manager_desktop_move_sel(0, -1); break;
                    case KEY_DOWN:  app_manager_desktop_move_sel(0, 1);  break;
                    case KEY_LEFT:  app_manager_desktop_move_sel(-1, 0); break;
                    case KEY_RIGHT: app_manager_desktop_move_sel(1, 0);  break;
                    case KEY_A:     app_manager_handle_a_press(); break;
                    case KEY_B:
                        // 当前是 App 屏 → 杀进程 + 返回桌面
                        if (app_manager_current() == SCREEN_APP) {
                            app_manager_kill_running();
                            app_manager_show(SCREEN_HOME);
                        } else {
                            app_manager_handle_b_press();
                        }
                        break;
                    case KEY_NUM1:  app_manager_editor_num(1); break;
                    case KEY_NUM2:  app_manager_editor_num(2); break;
                    case KEY_NUM3:  app_manager_editor_num(3); break;
                    case KEY_NUM4:  app_manager_editor_num(4); break;
                    case KEY_NUM5:  app_manager_editor_num(5); break;
                    case KEY_NUM6:  app_manager_editor_num(6); break;
                    case KEY_NUM7:  app_manager_editor_num(7); break;
                    case KEY_NUM8:  app_manager_editor_num(8); break;
                    case KEY_NUM9:  app_manager_editor_num(9); break;
                    case KEY_DEL:   app_manager_editor_delete(); break;
                    case KEY_INS:   app_manager_editor_insert(); break;
                    default: break;
                }
            } else {
                app_manager_handle_long_a_press();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ================ LVGL tick ================
static void lvgl_tick_task(void *arg)
{
    (void)arg;
    while (1) {
        lv_tick_inc(10);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void lvgl_task(void *arg)
{
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(33));   // ~30fps
    }
}

// ================ 把扫描到的 .app 加到桌面 ================
static int on_app_found(const app_info_t *a)
{
    ESP_LOGI(TAG, "scan: %s [%s] v%s path=%s",
             a->id, a->name, a->version, a->source_path ? a->source_path : "(builtin)");
    return 0;
}

// ================ app_main ================
void app_main(void)
{
    ESP_LOGI(TAG, "===== 小喵 OS v0.2.0 boot =====");
    ESP_LOGI(TAG, "Flash: %dMB, PSRAM: %dMB, Free heap: %d",
             4, 8, (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    // 0. 返回 Loader 兼容（务必先调用, 否则不能回原厂 ROM）
    return_to_loader_setup();
    return_to_loader_set_app_desc("xiaomiao-os", "Android-style desktop (MicroPython)");

    // 1. NVS
    ESP_ERROR_CHECK(nvs_flash_init());

    // 2. HAL
    ESP_ERROR_CHECK(lcd_init());
    ESP_ERROR_CHECK(keys_init());
    ESP_ERROR_CHECK(battery_init());
    ESP_ERROR_CHECK(buzzer_init());
    sdcard_init();        // SD 失败不致命
    mpu6050_init();       // IMU 失败不致命
    gd32_init();

    // 3. MicroPython 运行时（在 LVGL 之前初始化, 后面 hal.* 调用要它就绪）
    ESP_ERROR_CHECK(mp_runtime_init());

    // 4. LVGL 初始化
    lv_init();

    // 三缓冲
    s_buf1 = (lv_color_t *)lcd_front_buffer_take();
    s_buf2 = (lv_color_t *)lcd_front_buffer_take();
    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2, LV_HOR_RES * LV_VER_RES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = lcd_flush_cb;
    disp_drv.draw_buf = &s_draw_buf;
    disp_drv.hor_res = LV_HOR_RES;
    disp_drv.ver_res = LV_VER_RES;
    lv_disp_drv_register(&disp_drv);

    // 5. 主题
    theme_init();

    // 6. 顶层屏
    lv_obj_t *top = lv_obj_create(lv_scr_act());
    lv_obj_set_size(top, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(top, lv_color_hex(0xF6D34A), 0);
    lv_obj_set_style_pad_all(top, 0, 0);
    lv_obj_set_style_border_width(top, 0, 0);

    // 7. 业务层
    settings_init();
    app_manager_init(top);
    toast_init(top);
    input_init();

    // 8. 扫描 SD 卡 App
    app_loader_scan_app_dir_with_cb(on_app_found);

    // 9. 后台任务
    xTaskCreate(lvgl_tick_task, "lvgl_tick", 1024, NULL, 5, NULL);
    xTaskCreate(lvgl_task,       "lvgl",      4096, NULL, 4, NULL);
    xTaskCreate(key_dispatch_task, "keys",    2048, NULL, 5, NULL);
    xTaskCreate(statusbar_task,  "statusbar", 2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "===== 小喵 OS ready =====");
}