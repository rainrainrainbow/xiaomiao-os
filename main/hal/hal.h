#pragma once
/*
 * hal.h - 硬件抽象层统一入口
 */
#include "hal/lcd.h"
#include "hal/sdcard.h"
#include "hal/keys.h"
#include "hal/backlight.h"
#include "hal/battery.h"
#include "i2c_bus.h"
#include "gyro.h"
#include "buzzer.h"

esp_err_t hal_init_all(void);  /* 一次性初始化全部硬件 */
