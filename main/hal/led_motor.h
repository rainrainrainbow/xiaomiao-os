// ================ led_motor.h - GD32 LED/马达 控制 ================
// GD32 通过 USB-UART 桥接, UART0 TX=1, RX=3
// 协议: [STX=0xAA] [LEN] [CMD] [DATA...] [CHECKSUM]

#ifndef __LED_MOTOR_H__
#define __LED_MOTOR_H__

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t gd32_init(void);
bool     gd32_is_connected(void);

// LED: 0=灭 1=亮 2=呼吸
esp_err_t gd32_set_led(uint8_t mode);

// 马达: 0=停 1=轻 2=强
esp_err_t gd32_set_motor(uint8_t level);

#ifdef __cplusplus
}
#endif

#endif // __LED_MOTOR_H__