// ================ input.h - 按键 → LVGL 输入设备 ================

#ifndef __INPUT_H__
#define __INPUT_H__

#include "lvgl.h"
#include "hal/keys.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册 LVGL 键盘输入设备
 *        自动将按键事件转发到 LVGL keypad group
 *
 * 用法：
 *   1. lv_indev_t *kb = input_init();
 *   2. lv_indev_set_group(kb, my_group);
 */
lv_indev_t *input_init(void);

/**
 * @brief 把 LVGL key 转 enum（与 keys.h 中的 code 对应）
 */
uint32_t keycode_from_key(key_code_t code);

#ifdef __cplusplus
}
#endif

#endif // __INPUT_H__