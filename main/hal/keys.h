#pragma once
#include <stdbool.h>

/* 按键引脚定义（低电平有效） */
#define KEY_UP_PIN     2
#define KEY_DOWN_PIN   13
#define KEY_LEFT_PIN   27
#define KEY_RIGHT_PIN  35   /* GPIO35: 仅输入, 无内部上拉 */
#define KEY_A_PIN      34   /* GPIO34: 仅输入, 无内部上拉 (电池检测复用) */
#define KEY_B_PIN      12

/* 按键枚举 */
typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_A,
    KEY_B,
    KEY_COUNT
} key_code_t;

/* 按键事件类型 */
typedef enum {
    KEY_EVENT_PRESS = 0,    /* 刚按下 */
    KEY_EVENT_RELEASE,      /* 刚释放 */
    KEY_EVENT_HOLD,         /* 长按中 */
} key_event_t;

/* 初始化 / 轮询 */
esp_err_t keys_init(void);
key_code_t keys_scan(void);          /* 返回当前按下的键（带消抖） */
bool       key_is_pressed(key_code_t k);
void       keys_update(void);        /* 每 10ms 调用一次，更新状态机 */

/* 回调注册 */
typedef void (*key_callback_t)(key_code_t key, key_event_t event);
void       keys_set_callback(key_callback_t cb);

/* 电池检测（KEY_A 引脚复用为 ADC） */
void       keys_battery_adc_init(void);
uint16_t   keys_battery_read_raw(void);  /* 12-bit ADC 原始值 */
float      keys_battery_voltage(void);    /* 计算实际电池电压 */
