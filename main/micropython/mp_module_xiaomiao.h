#pragma once
#include "esp_err.h"

/* xiaomiao 系统模块 - MicroPython C 扩展
 *
 * 注册为 Python 模块，供用户脚本调用:
 *   import xiaomiao as xm
 *   xm.screen.text("Hello")
 *   xm.key.a_pressed()
 *   xm.sensor.battery()
 *   xm.music.tone(440, 0.5)
 *   xm.storage.write("save.dat", data)
 *   xm.system.exit()
 *   xm.time.sleep(1)
 */

esp_err_t mp_module_xiaomiao_register(void);

/* 子模块 */
void mp_module_screen_register(void);   /* xiaomiao.screen */
void mp_module_key_register(void);      /* xiaomiao.key */
void mp_module_sensor_register(void);   /* xiaomiao.sensor */
void mp_module_music_register(void);    /* xiaomiao.music */
void mp_module_storage_register(void);  /* xiaomiao.storage */
void mp_module_system_register(void);   /* xiaomiao.system */
void mp_module_time_register(void);     /* xiaomiao.time */
