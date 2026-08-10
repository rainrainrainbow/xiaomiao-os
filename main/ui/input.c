// ================ input.c - LVGL 键盘设备 + 按键事件 ================

#include "input.h"
#include "hal/keys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ui.input";

uint32_t keycode_from_key(key_code_t code)
{
    switch (code) {
        case KEY_UP:    return LV_KEY_UP;
        case KEY_DOWN:  return LV_KEY_DOWN;
        case KEY_LEFT:  return LV_KEY_LEFT;
        case KEY_RIGHT: return LV_KEY_RIGHT;
        case KEY_A:     return LV_KEY_ENTER;
        case KEY_B:     return LV_KEY_ESC;
        case KEY_DEL:   return LV_KEY_BACKSPACE;
        default:        return 0;
    }
}

// LVGL 9 的 input 回调（按键状态机）
static void keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static key_code_t s_last;
    static bool       s_pressed;

    key_event_t e;
    if (keys_pop(&e)) {
        if (e.code != KEY_NONE) {
            s_last = e.code;
            s_pressed = !e.long_press;  // 长按不重复
        }
    }

    data->key = keycode_from_key(s_last);
    data->state = (s_pressed && keys_is_pressed(s_last)) ? LV_INDEV_STATE_PRESSED
                                                          : LV_INDEV_STATE_RELEASED;

    if (data->state == LV_INDEV_STATE_RELEASED) {
        s_pressed = false;
        s_last = KEY_NONE;
        data->key = 0;
    }
}

static lv_indev_t *s_indev = NULL;

lv_indev_t *input_init(void)
{
    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_indev, keypad_read_cb);
    ESP_LOGI(TAG, "LVGL keypad input init OK");
    return s_indev;
}