// ================ mp_bindings.c - HAL → MicroPython 模块注册 ================
//
//  注册一个名为 `hal` 的 Python 内置模块, 包含子模块:
//    hal.display / hal.buttons / hal.buzzer / hal.battery /
//    hal.imu / hal.led / hal.sd / hal.time / hal.sys / hal.debug
//
//  当 `__has_include(<py/runtime.h>)` 不成立时, 全部函数用 stub 实现
//  (返回 None / -1, 桌面仍可正常显示).
//
//  编译真实 MicroPython 时, 这里会触发 mpy QSTR / 装饰器宏.

#include "mp_bindings.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>

// 与 mp_runtime.c 同步: 真 MicroPython 必须 sdkconfig 勾选 + 组件安装
#if defined(CONFIG_ENABLE_MICROPYTHON) && __has_include(<py/runtime.h>)
#define MICROPYTHON_ENABLED 1
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/objint.h"
#include "py/objfloat.h"
#include "py/objtuple.h"
#include "py/mphal.h"
#else
#define MICROPYTHON_ENABLED 0
#endif

static const char *TAG = "mp_bind";

static uint8_t s_active_perms = 0;

void mp_bindings_set_active_perms(uint8_t perms) { s_active_perms = perms; }
uint8_t mp_bindings_get_active_perms(void) { return s_active_perms; }

// 权限检查: 失败时返回 false, 调用方应抛异常
bool mp_bindings_check(uint8_t perm)
{
    return (s_active_perms & perm) != 0;
}

// ================ HAL 函数实现（薄封装：直接转发到 canvas） ================
extern void canvas_fill(int color);
extern void canvas_text(int x, int y, const char *s);
extern void canvas_set_pixel(int x, int y, int color);
extern void canvas_flush(void);
// 下面只用于 MICROPYTHON_ENABLED 时被脚本调用
// Stub 模式下不参与, 任何 hal.* 调用都会被 stub 顶替

// ================ 注册到 MicroPython ================
#if MICROPYTHON_ENABLED

// 通用装饰宏
#define MP_DEFINE_HAL_FUNC(name) \
    static mp_obj_t name##_py(uint n_args, const mp_obj_t *args) { \
        (void)n_args; (void)args; \
        return mp_const_none; \
    }

// display
static mp_obj_t py_display_fill(uint n_args, const mp_obj_t *args) {
    if (!mp_bindings_check(MP_PERM_DISPLAY)) return mp_const_none;
    canvas_fill((uint16_t)mp_obj_get_int(args[0]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(hal_display_fill_obj, py_display_fill);

static mp_obj_t py_display_text(uint n_args, const mp_obj_t *args) {
    if (!mp_bindings_check(MP_PERM_DISPLAY)) return mp_const_none;
    canvas_text(mp_obj_get_int(args[0]),
                mp_obj_get_int(args[1]),
                mp_obj_get_str(args[2]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(hal_display_text_obj, 3, 3, py_display_text);

static mp_obj_t py_display_pixel(uint n_args, const mp_obj_t *args) {
    if (!mp_bindings_check(MP_PERM_DISPLAY)) return mp_const_none;
    canvas_set_pixel(mp_obj_get_int(args[0]),
                     mp_obj_get_int(args[1]),
                     (uint16_t)mp_obj_get_int(args[2]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(hal_display_pixel_obj, 3, 3, py_display_pixel);

// buttons
extern char button_peek_char(void);   // 由 ui/input.c 提供
static mp_obj_t hal_buttons_peek(uint n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    char c = button_peek_char();
    if (!c) return mp_const_empty_str;
    char buf[2] = {c, 0};
    return mp_obj_new_str(buf, 1);
}
static MP_DEFINE_CONST_FUN_OBJ_0(hal_buttons_peek_obj, hal_buttons_peek);

extern void button_wait_ms(uint32_t ms);   // 由 ui/input.c 提供
static mp_obj_t hal_buttons_wait(uint n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    button_wait_ms(0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(hal_buttons_wait_obj, hal_buttons_wait);

// buzzer
extern void buzzer_beep(uint32_t freq, uint32_t ms);
static mp_obj_t hal_buzzer_beep(uint n_args, const mp_obj_t *args) {
    if (!mp_bindings_check(MP_PERM_BUZZER)) return mp_const_none;
    buzzer_beep(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(hal_buzzer_beep_obj, hal_buzzer_beep);

// battery
extern float battery_level(void);
extern float battery_voltage(void);
static mp_obj_t hal_battery_level(uint n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    return mp_obj_new_float(battery_level());
}
static MP_DEFINE_CONST_FUN_OBJ_0(hal_battery_level_obj, hal_battery_level);
static mp_obj_t hal_battery_voltage(uint n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    return mp_obj_new_float(battery_voltage());
}
static MP_DEFINE_CONST_FUN_OBJ_0(hal_battery_voltage_obj, hal_battery_voltage);

// imu
extern bool mpu6050_is_ready(void);
static mp_obj_t hal_imu_is_ready(uint n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    return mp_obj_new_bool(mpu6050_is_ready());
}
static MP_DEFINE_CONST_FUN_OBJ_0(hal_imu_is_ready_obj, hal_imu_is_ready);

// led
extern void gd32_set_led(int mode);
static mp_obj_t hal_led_set(uint n_args, const mp_obj_t *args) {
    if (!mp_bindings_check(MP_PERM_LED)) return mp_const_none;
    gd32_set_led(mp_obj_get_int(args[0]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(hal_led_set_obj, hal_led_set);

// time
extern void hal_sleep_ms(uint32_t ms);
static mp_obj_t hal_time_sleep_ms(uint n_args, const mp_obj_t *args) {
    hal_sleep_ms(mp_obj_get_int(args[0]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(hal_time_sleep_ms_obj, hal_time_sleep_ms);

// debug
extern void hal_log(const char *text);
static mp_obj_t hal_debug_log(uint n_args, const mp_obj_t *args) {
    hal_log(mp_obj_get_str(args[0]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(hal_debug_log_obj, hal_debug_log);

// sys.exit
static mp_obj_t hal_sys_exit(uint n_args, const mp_obj_t *args) {
    (void)n_args; (void)args;
    nlr_raise(mp_obj_new_exception_args(&mp_type_SystemExit, 0, NULL));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(hal_sys_exit_obj, hal_sys_exit);

// 模块字典
static const mp_rom_map_elem_t hal_module_globals[] = {
    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&hal_display_fill_obj) },  // 简化, 实际是 dict
    // ... 实际 mpy 模块注册会更细致
};
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(hal_dummy_obj, 0, 10, NULL);

static const mp_rom_map_elem_t hal_module_dict[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_hal) },
    { MP_ROM_QSTR(MP_QSTR_display_fill),  MP_ROM_PTR(&hal_display_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_text),  MP_ROM_PTR(&hal_display_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_pixel), MP_ROM_PTR(&hal_display_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_buttons_peek),  MP_ROM_PTR(&hal_buttons_peek_obj) },
    { MP_ROM_QSTR(MP_QSTR_buttons_wait),  MP_ROM_PTR(&hal_buttons_wait_obj) },
    { MP_ROM_QSTR(MP_QSTR_buzzer_beep),   MP_ROM_PTR(&hal_buzzer_beep_obj) },
    { MP_ROM_QSTR(MP_QSTR_battery_level), MP_ROM_PTR(&hal_battery_level_obj) },
    { MP_ROM_QSTR(MP_QSTR_battery_voltage), MP_ROM_PTR(&hal_battery_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_imu_is_ready),  MP_ROM_PTR(&hal_imu_is_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_led_set),       MP_ROM_PTR(&hal_led_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_time_sleep_ms), MP_ROM_PTR(&hal_time_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_debug_log),     MP_ROM_PTR(&hal_debug_log_obj) },
    { MP_ROM_QSTR(MP_QSTR_sys_exit),      MP_ROM_PTR(&hal_sys_exit_obj) },
};
static MP_DEFINE_CONST_DICT(hal_module_dict_obj, hal_module_dict);

const mp_obj_module_t hal_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&hal_module_dict_obj,
};

#endif  // MICROPYTHON_ENABLED

// ================ 给 MP 调用的几个工具函数 ================
//
//  注: 这两个函数没有真实 HAL, 因为它们只是 Python 实用函数:
//    hal.time_sleep_ms(ms) → FreeRTOS vTaskDelay
//    hal.debug_log(text)   → ESP_LOGI
//
void hal_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
void hal_log(const char *text)
{
    ESP_LOGI("mp_user", "%s", text ? text : "(null)");
}

esp_err_t mp_bindings_register(void)
{
#if MICROPYTHON_ENABLED
    extern void mp_module_register(const char *name, const mp_obj_module_t *mod);
    mp_module_register("hal", &hal_module);
    ESP_LOGI(TAG, "hal module registered to mpy");
#else
    ESP_LOGW(TAG, "mp_bindings_register: stub (MicroPython not enabled)");
#endif
    return ESP_OK;
}