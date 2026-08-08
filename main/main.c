/*
 * xiaomiao-os — 小喵掌机 MicroPython 桌面系统
 *
 * Phase 1: 环境搭建与基础硬件验证
 *
 * ESP32-WROVER-B + ST7735 160x128 TFT + MicroSD + 6-key keypad +
 * I2C (GD32/MPU6050) + Buzzer + Battery ADC
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "sdkconfig.h"

#include "xiaomiao_hal.h"
#include "app_manager.h"
#include "return_to_loader.h"

static const char *TAG = "XOS_MAIN";

/* ── UI: Self-Test Screen ──────────────────────────────────────── */

static lv_obj_t *s_scr_test;
static lv_obj_t *s_lbl_title;
static lv_obj_t *s_lbl_results;
static lv_obj_t *s_lbl_hint;
static xos_hal_status_t s_hal_status;

static void ui_update_test_results(void)
{
    char buf[512];
    int pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "LCD:    %s\n"
        "SD:     %s\n"
        "I2C:    %s\n"
        "Buzzer: %s\n"
        "Btns:   %s\n"
        "Batt:   %s\n"
        "\n"
        "Voltage: %.2fV\n"
        "Battery: %d%%\n",
        s_hal_status.lcd_ok    ? "OK" : "FAIL",
        s_hal_status.sd_ok      ? "OK" : "NO CARD",
        s_hal_status.i2c_ok     ? "OK" : "FAIL",
        s_hal_status.buzzer_ok  ? "OK" : "FAIL",
        s_hal_status.buttons_ok ? "OK" : "FAIL",
        s_hal_status.battery_ok ? "OK" : "FAIL",
        s_hal_status.battery_voltage,
        s_hal_status.battery_percent);

    lv_label_set_text(s_lbl_results, buf);
}

static void ui_create_test_screen(lv_display_t *disp)
{
    s_scr_test = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_test, lv_color_hex(XOS_CLR_YELLOW), 0);
    lv_obj_set_style_bg_opa(s_scr_test, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_scr_test, 0, 0);
    lv_obj_set_flex_flow(s_scr_test, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_scr_test, 0, 0);

    /* Title bar */
    s_lbl_title = lv_label_create(s_scr_test);
    lv_obj_set_width(s_lbl_title, XOS_LCD_H_RES);
    lv_obj_set_height(s_lbl_title, 14);
    lv_obj_set_style_bg_color(s_lbl_title, lv_color_hex(XOS_CLR_BROWN), 0);
    lv_obj_set_style_bg_opa(s_lbl_title, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_lbl_title, lv_color_hex(XOS_CLR_CREAM), 0);
    lv_label_set_text(s_lbl_title, " XiaoMiao OS  HW Test");
    lv_obj_set_style_pad_left(s_lbl_title, 2, 0);
    lv_obj_set_style_pad_ver(s_lbl_title, 2, 0);

    /* Results area */
    s_lbl_results = lv_label_create(s_scr_test);
    lv_obj_set_width(s_lbl_results, XOS_LCD_H_RES);
    lv_obj_set_flex_grow(s_lbl_results, 1);
    lv_obj_set_style_text_color(s_lbl_results, lv_color_hex(XOS_CLR_BLACK), 0);
    lv_obj_set_style_pad_left(s_lbl_results, 4, 0);
    lv_obj_set_style_pad_top(s_lbl_results, 2, 0);
    lv_label_set_text(s_lbl_results, "Testing...");

    /* Hint bar */
    s_lbl_hint = lv_label_create(s_scr_test);
    lv_obj_set_width(s_lbl_hint, XOS_LCD_H_RES);
    lv_obj_set_height(s_lbl_hint, 12);
    lv_obj_set_style_bg_color(s_lbl_hint, lv_color_hex(XOS_CLR_BROWN), 0);
    lv_obj_set_style_bg_opa(s_lbl_hint, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_lbl_hint, lv_color_hex(XOS_CLR_CREAM), 0);
    lv_label_set_text(s_lbl_hint, " A:Beep B:Re-test");
    lv_obj_set_style_pad_left(s_lbl_hint, 2, 0);
    lv_obj_set_style_pad_ver(s_lbl_hint, 2, 0);

    lv_screen_load(s_scr_test);
}

/* ── Key Event Handler ─────────────────────────────────────────── */

static void on_key(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER) {
        /* A button: beep test */
        xos_buzzer_set_freq(988);  /* B5 */
        vTaskDelay(pdMS_TO_TICKS(150));
        xos_buzzer_off();
    } else if (key == LV_KEY_ESC) {
        /* B button: re-run self-test */
        xos_hal_self_test(&s_hal_status);
        s_hal_status.battery_voltage = xos_battery_read_voltage();
        s_hal_status.battery_percent = xos_battery_read_percent();
        ui_update_test_results();
    }
}

/* ── Main ──────────────────────────────────────────────────────── */

void app_main(void)
{
    /* Return-to-Loader: must be first line */
    return_to_loader_setup();

    ESP_LOGI(TAG, "XiaoMiao OS v0.1.0 - Phase 1: Hardware Verification");

    /* Initialize all hardware */
    esp_err_t ret = xos_hal_init(&s_hal_status);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HAL init failed: %s", esp_err_to_name(ret));
    }

    /* Run self-test and log results */
    xos_hal_self_test(&s_hal_status);

    /* Initialize app manager */
    xos_appmgr_init();
    if (s_hal_status.sd_ok) {
        xos_app_list_t app_list;
        xos_appmgr_scan(&app_list);
        ESP_LOGI(TAG, "Found %d apps on SD card", app_list.count);
    }

    /* Initialize LVGL */
    lv_init();
    lv_display_t *disp = xos_display_init();

    /* Create keypad input device */
    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_display(indev, disp);
    lv_indev_set_group(indev, group);
    lv_indev_set_read_cb(indev, xos_keypad_read_cb);
    lv_indev_set_long_press_time(indev, 360);
    lv_indev_set_long_press_repeat_time(indev, 130);

    /* Create self-test UI */
    ui_create_test_screen(disp);
    ui_update_test_results();

    /* Register key event on the screen object */
    lv_obj_add_event_cb(s_scr_test, on_key, LV_EVENT_KEY, NULL);
    lv_group_add_obj(group, s_scr_test);

    /* Enable display */
    lv_refr_now(NULL);

    /* Wait for first flush to complete */
    for (int i = 0; i < 100; i++) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    /* Turn on display */
    esp_lcd_panel_io_handle_t io = xos_lcd_get_io();
    if (io) {
        esp_lcd_panel_io_tx_param(io, 0x29, NULL, 0); /* DISPON */
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ESP_LOGI(TAG, "System ready. A=beep test, B=re-test.");

    /* Main loop */
    while (true) {
        uint32_t delay = lv_timer_handler();
        usleep(MAX(MIN(delay, 16), 1) * 1000);
    }
}