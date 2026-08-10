// ================ keys.h - 6部分。 键 + 长按检测 ================
// 按键: UP=2  DOWN=13  LEFT=27  RIGHT=35  A=34  B=12

#ifndef __KEYS_H__
#define __KEYS_H__

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ================ 按键编码（与 LVGL 转译一致） ================
typedef enum {
    KEY_NONE  = 0,
    KEY_UP    = 1,
    KEY_DOWN  = 2,
    KEY_LEFT  = 3,
    KEY_RIGHT = 4,
    KEY_A     = 5,
    KEY_B     = 6,
    KEY_NUM1  = 11,  // 编辑器快捷键
    KEY_NUM2  = 12,
    KEY_NUM3  = 13,
    KEY_NUM4  = 14,
    KEY_NUM5  = 15,
    KEY_NUM6  = 16,
    KEY_NUM7  = 17,
    KEY_NUM8  = 18,
    KEY_NUM9  = 19,
    KEY_DEL   = 20,
    KEY_INS   = 21,
} key_code_t;

typedef struct {
    key_code_t code;
    bool       long_press;  // true = 长按触发
} key_event_t;

/**
 * @brief 初始化按键 GPIO + 扫描 timer
 */
esp_err_t keys_init(void);

/**
 * @brief 取最近一次事件（队列）
 * @return true 有事件已写入 *out
 */
bool keys_pop(key_event_t *out);

/**
 * @brief 同步获取当前按键状态（用于 ROM 切换 / 长按组合）
 */
bool keys_is_pressed(key_code_t code);

/**
 * @brief 长按 A 360ms 阈值（MV 设定）
 */
#define KEY_LONG_PRESS_MS  360

#ifdef __cplusplus
}
#endif

#endif // __KEYS_H__