#pragma once
#include <stdbool.h>

#define BATTERY_ADC_PIN   34   /* GPIO34 = ADC1_CH6 */
#define BATTERY_R1_KOHM   9.1f /* 上拉电阻 */
#define BATTERY_R2_KOHM   2.4f /* 下拉电阻 */

esp_err_t battery_init(void);
float      battery_voltage(void);       /* 电池电压 (V) */
uint8_t    battery_percentage(void);    /* 0-100% */
bool       battery_is_low(void);        /* < 3.3V 警告 */
const char *battery_status_text(void);  /* "满电"/"正常"/"低电" */
