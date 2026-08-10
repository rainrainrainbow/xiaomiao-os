// ================ battery.h - 电池电压 + A 键复用检测 ================
//
//  硬件复用:
//    GPIO34 / ADC1_CH6 通过分压电阻测电池电压
//       9.1k (上拉到 BAT+) + 2.4k (下拉到 GND)
//       V_adc = V_bat × 2.4 / (9.1 + 2.4) = V_bat × 0.2086
//       → V_bat = V_adc × 4.7917
//
//    A 键按压: 直接把 2.4k 短路到 GND 旁路 → ADC 读到 ≈ 0V
//       阈值: V_adc < 0.3V → A 按下
//             V_adc > 0.5V → A 未按, 正常测电池
//
//  注: 该电路实测真实存在 (ZYoungInc 原理图), 分压点本身就
//       是 A 键开关的下拉网络. 不需要专门 A 键 GPIO.

#ifndef __BATTERY_H__
#define __BATTERY_H__

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_ADC_CHANNEL  ADC_CHANNEL_6   // GPIO34 ADC1_CH6
#define BATTERY_GPIO_NUM     34

// 分压电阻 (kΩ)
#define BATTERY_R_TOP   9.1f
#define BATTERY_R_BOT   2.4f
#define BATTERY_DIVIDER_RATIO  ((BATTERY_R_TOP + BATTERY_R_BOT) / BATTERY_R_BOT)   // ≈ 4.792

// A 键按下时 ADC 阈值 (V)
#define A_KEY_DOWN_THRESHOLD  0.30f
#define A_KEY_UP_THRESHOLD    0.50f   // 迟滞避免抖动

esp_err_t battery_init(void);

/** 周期调用 (keys.c 调用), 更新 A 键迟滞状态 */
void battery_a_update(void);

/** 电池电压 V，3.0~4.2 区间 */
float battery_voltage(void);

/** 电量 0.0~1.0 */
float battery_level(void);

/** A 键当前状态（双阈值迟滞） */
bool battery_a_is_pressed(void);

/** A 键 ADC 原始电压（debug） */
float battery_a_adc_v(void);

#ifdef __cplusplus
}
#endif

#endif // __BATTERY_H__