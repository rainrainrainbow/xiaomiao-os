// ================ settings.h - 设置持久化（NVS） ================

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t settings_init(void);

bool    settings_wifi_on(void);
int     settings_volume(void);          // 0-100
float   settings_battery_v(void);       // 电池电压
void    settings_note_user_app(void);   // 用户首次生成 App 时打标记

/**
 * @brief 设置项列表循环切换（按下 A 时调用）
 * @param idx 0=Wi-Fi 1=音量 2=亮度 3=壁纸 4=语言 5=电池 6=关于
 */
void settings_advance(int idx);

#ifdef __cplusplus
}
#endif

#endif // __SETTINGS_H__