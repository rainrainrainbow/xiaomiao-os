/**
 * @file lvgl_port.h
 * @brief LVGL 移植层 - 显示驱动、按键输入和组合键检测
 */

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "lvgl.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 按键枚举
 */
typedef enum {
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_A,
    BTN_B,
    BTN_COUNT
} button_id_t;

/**
 * @brief 组合键回调类型
 */
typedef void (*combo_key_callback_t)(void);

/**
 * @brief 按键事件回调类型
 */
typedef void (*key_event_callback_t)(lv_key_t key);

/**
 * @brief 初始化 LVGL 移植层（显示驱动 + 按键输入）
 * @param lcd_io LCD 面板 IO 句柄
 * @param lcd_panel LCD 面板句柄
 * @return ESP_OK 成功
 */
esp_err_t lvgl_port_init(esp_lcd_panel_io_handle_t lcd_io, esp_lcd_panel_handle_t lcd_panel);

/**
 * @brief LVGL 任务处理（在主循环中调用）
 */
void lvgl_port_task_handler(void);

/**
 * @brief 初始化按键输入
 * @return ESP_OK 成功
 */
esp_err_t lvgl_port_input_init(void);

/**
 * @brief 注册组合键回调
 * @param combo 组合键类型: "up+b" 或 "down+b"
 * @param callback 回调函数
 */
void lvgl_port_register_combo_key(const char *combo, combo_key_callback_t callback);

/**
 * @brief 注册按键事件回调（单键按下时触发）
 * @param callback 回调函数
 */
void lvgl_port_register_key_event(key_event_callback_t callback);

/**
 * @brief LVGL 按键读取回调（供 LVGL 输入设备使用）
 */
void lvgl_port_keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data);

/**
 * @brief 获取按键状态
 * @param btn 按键ID
 * @return true 按下，false 释放
 */
bool lvgl_port_is_button_pressed(button_id_t btn);

/**
 * @brief 按键扫描任务（需要在 FreeRTOS 任务中调用）
 */
void lvgl_port_button_scan_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_PORT_H */