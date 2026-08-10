// ================ buzzer.h - LEDC Timer0/Ch0 蜂鸣器 ================

#ifndef __BUZZER_H__
#define __BUZZER_H__

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUZZER_PIN  14  // GPIO14 LEDC

esp_err_t buzzer_init(void);
void buzzer_beep(uint32_t freq, uint32_t duration_ms);
void buzzer_click(void);   // 短促"哒" 2kHz 20ms
void buzzer_success(void); // 上升 1k→2k
void buzzer_fail(void);    // 下降 2k→500

#ifdef __cplusplus
}
#endif

#endif // __BUZZER_H__