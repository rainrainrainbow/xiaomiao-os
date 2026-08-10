// ================ mp_bindings.h - HAL 暴露给 MicroPython ================
//
//  Python 端 `import hal` 可调用的函数:
//    display.fill(color)
//    display.text(x, y, text)
//    display.draw(rect, color)
//    display.flush()                # LVGL 推送
//    display.set_pixel(x, y, color)
//    buttons.peek()                  # → char / '' (无按键)
//    buttons.wait()                  # 阻塞直到下一次按键
//    buzzer.beep(freq_hz, ms)
//    battery.level()                 # → 0.0~1.0
//    battery.voltage()               # → V
//    imu.read()                      # → (ax, ay, az, gx, gy, gz, t)
//    imu.is_ready()                  # → bool
//    led.set(mode)                   # 0=灭 1=亮 2=呼吸
//    sd.list(dir)                    # → list
//    sd.read(path)                   # → bytes
//    time.sleep_ms(ms)
//    sys.app_id()                    # → 当前 App id
//    sys.exit()                      # 终止当前 App
//    debug.log(text)
//
//  权限系统:
//    启动时把 manifest 的 permissions 传给本模块, 由
//    mp_bindings_gate_check() 决定函数是否抛 PermissionError

#ifndef __MP_BINDINGS_H__
#define __MP_BINDINGS_H__

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 权限位
enum {
    MP_PERM_DISPLAY = 1 << 0,
    MP_PERM_BUTTONS = 1 << 1,
    MP_PERM_BUZZER  = 1 << 2,
    MP_PERM_BATTERY = 1 << 3,
    MP_PERM_IMU     = 1 << 4,
    MP_PERM_LED     = 1 << 5,
    MP_PERM_SD      = 1 << 6,
    MP_PERM_NET     = 1 << 7,
    MP_PERM_ALL     = 0xFF,
};

void mp_bindings_set_active_perms(uint8_t perms);
uint8_t mp_bindings_get_active_perms(void);

/** 注册 Python 模块 `hal` 到 mpy — 在 mpy_init 之后调用 */
esp_err_t mp_bindings_register(void);

#ifdef __cplusplus
}
#endif

#endif // __MP_BINDINGS_H__