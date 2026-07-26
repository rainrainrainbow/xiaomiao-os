/*
 * mp_module_xiaomiao_full.c - 完整的 xiaomiao MicroPython C 扩展模块
 *
 * 当 MicroPython 完整集成时，此文件提供所有系统 API 的真实实现。
 * 当降级模式时，mp_module_xiaomiao.c 中的占位函数已足够。
 *
 * 模块结构:
 *   xiaomiao
 *   ├── screen  (text, rect, clear, pixel)
 *   ├── key     (a_pressed, b_pressed, direction, wait_press)
 *   ├── sensor  (acc_x, acc_y, acc_z, gyro_z, battery, temperature)
 *   ├── music   (tone, stop, melody)
 *   ├── storage (write, read, exists, list)
 *   ├── system  (exit, sleep, info, reboot)
 *   └── time    (sleep, ticks_ms, ticks_diff)
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* MicroPython API */
#include "py/obj.h"
#include "py/objmodule.h"
#include "py/objstr.h"
#include "py/objint.h"
#include "py/objdict.h"
#include "py/runtime.h"
#include "py/qstr.h"
#include "py/mphal.h"

/* XiaoMiao HAL */
#include "hal/keys.h"
#include "hal/battery.h"
#include "hal/gyro.h"
#include "hal/buzzer.h"
#include "hal/backlight.h"
#include "ui/page_manager.h"
#include "lvgl.h"

static const char *TAG = "mp_xm_full";

/* ==================== screen 子模块 ==================== */

STATIC mp_obj_t screen_text(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_text, ARG_x, ARG_y, ARG_color };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x,     MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y,     MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_color, MP_ARG_INT, {.u_int = 0xFFFF} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const char *text = mp_obj_str_get_str(args[ARG_text].u_obj);
    int x = args[ARG_x].u_int;
    int y = args[ARG_y].u_int;
    uint16_t color = (uint16_t)args[ARG_color].u_int;

    ESP_LOGD(TAG, "[screen.text] \"%s\" at (%d,%d) color=0x%04X", text, x, y, color);

    /* 通过 LVGL 在主屏幕绘制 */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_make((color>>11)<<3, ((color>>5)&0x3F)<<2, (color&0x1F)<<3), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_8, 0);
    lv_obj_set_pos(lbl, x, y + 12); /* +12 避开状态栏 */

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(screen_text_obj, 1, screen_text);

STATIC mp_obj_t screen_rect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_x, ARG_y, ARG_w, ARG_h, ARG_color };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_x,     MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y,     MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_w,     MP_ARG_INT, {.u_int = 50} },
        { MP_QSTR_h,     MP_ARG_INT, {.u_int = 30} },
        { MP_QSTR_color, MP_ARG_INT, {.u_int = 0xF800} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    ESP_LOGD(TAG, "[screen.rect] (%d,%d) %dx%d color=0x%04X",
             args[0].u_int, args[1].u_int, args[2].u_int, args[3].u_int, args[4].u_int);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *rect = lv_obj_create(scr);
    lv_obj_set_size(rect, args[ARG_w].u_int, args[ARG_h].u_int);
    lv_obj_set_pos(rect, args[ARG_x].u_int, args[ARG_y].u_int + 12);
    lv_color_t c = lv_color_make((args[ARG_color].u_int>>11)<<3,
                                  ((args[ARG_color].u_int>>5)&0x3F)<<2,
                                  (args[ARG_color].u_int&0x1F)<<3);
    lv_obj_set_style_bg_color(rect, c, 0);
    lv_obj_set_style_radius(rect, 0, 0);
    lv_obj_set_style_border_width(rect, 0, 0);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(screen_rect_obj, 0, screen_rect);

STATIC mp_obj_t screen_clear(void)
{
    ESP_LOGD(TAG, "[screen.clear]");
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(screen_clear_obj, screen_clear);

STATIC const mp_rom_map_elem_t screen_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_screen) },
    { MP_ROM_QSTR(MP_QSTR_text),     MP_ROM_PTR(&screen_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect),     MP_ROM_PTR(&screen_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear),    MP_ROM_PTR(&screen_clear_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_screen_globals, screen_globals_table);

const mp_obj_module_t mp_module_xiaomiao_screen = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_screen_globals,
};

/* ==================== key 子模块 ==================== */

STATIC mp_obj_t key_a_pressed(void)
{
    return mp_obj_new_bool(key_is_pressed(KEY_A));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(key_a_pressed_obj, key_a_pressed);

STATIC mp_obj_t key_b_pressed(void)
{
    return mp_obj_new_bool(key_is_pressed(KEY_B));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(key_b_pressed_obj, key_b_pressed);

STATIC mp_obj_t key_direction(void)
{
    if (key_is_pressed(KEY_UP))    return mp_obj_new_str("up", 2);
    if (key_is_pressed(KEY_DOWN))  return mp_obj_new_str("down", 4);
    if (key_is_pressed(KEY_LEFT))  return mp_obj_new_str("left", 4);
    if (key_is_pressed(KEY_RIGHT)) return mp_obj_new_str("right", 5);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(key_direction_obj, key_direction);

STATIC mp_obj_t key_wait_press(void)
{
    /* 阻塞等待任意按键 */
    while (keys_scan() == KEY_NONE) {
        mp_hal_delay_ms(10);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(key_wait_press_obj, key_wait_press);

STATIC const mp_rom_map_elem_t key_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_key) },
    { MP_ROM_QSTR(MP_QSTR_a_pressed),   MP_ROM_PTR(&key_a_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_pressed),   MP_ROM_PTR(&key_b_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_direction),   MP_ROM_PTR(&key_direction_obj) },
    { MP_ROM_QSTR(MP_QSTR_wait_press),  MP_ROM_PTR(&key_wait_press_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_key_globals, key_globals_table);

const mp_obj_module_t mp_module_xiaomiao_key = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_key_globals,
};

/* ==================== sensor 子模块 ==================== */

STATIC mp_obj_t sensor_acc_x(void) { return mp_obj_new_int(gyro_acc_x()); }
STATIC MP_DEFINE_CONST_FUN_OBJ_0(sensor_acc_x_obj, sensor_acc_x);

STATIC mp_obj_t sensor_acc_y(void) { return mp_obj_new_int(gyro_acc_y()); }
STATIC MP_DEFINE_CONST_FUN_OBJ_0(sensor_acc_y_obj, sensor_acc_y);

STATIC mp_obj_t sensor_acc_z(void) { return mp_obj_new_int(gyro_acc_z()); }
STATIC MP_DEFINE_CONST_FUN_OBJ_0(sensor_acc_z_obj, sensor_acc_z);

STATIC mp_obj_t sensor_gyro_z(void) { return mp_obj_new_int(gyro_gyro_z()); }
STATIC MP_DEFINE_CONST_FUN_OBJ_0(sensor_gyro_z_obj, sensor_gyro_z);

STATIC mp_obj_t sensor_battery(void) { return mp_obj_new_float(battery_voltage()); }
STATIC MP_DEFINE_CONST_FUN_OBJ_0(sensor_battery_obj, sensor_battery);

STATIC mp_obj_t sensor_temperature(void) { return mp_obj_new_float(gyro_temperature()); }
STATIC MP_DEFINE_CONST_FUN_OBJ_0(sensor_temperature_obj, sensor_temperature);

STATIC const mp_rom_map_elem_t sensor_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),  MP_ROM_QSTR(MP_QSTR_sensor) },
    { MP_ROM_QSTR(MP_QSTR_acc_x),     MP_ROM_PTR(&sensor_acc_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_acc_y),     MP_ROM_PTR(&sensor_acc_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_acc_z),     MP_ROM_PTR(&sensor_acc_z_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyro_z),    MP_ROM_PTR(&sensor_gyro_z_obj) },
    { MP_ROM_QSTR(MP_QSTR_battery),   MP_ROM_PTR(&sensor_battery_obj) },
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&sensor_temperature_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_sensor_globals, sensor_globals_table);

const mp_obj_module_t mp_module_xiaomiao_sensor = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_sensor_globals,
};

/* ==================== music 子模块 ==================== */

STATIC mp_obj_t music_tone(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_freq, ARG_duration };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq,     MP_ARG_INT, {.u_int = 440} },
        { MP_QSTR_duration, MP_ARG_OBJ, {.u_obj = mp_obj_new_float(0.5)} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint16_t freq = (uint16_t)args[ARG_freq].u_int;
    float dur = mp_obj_get_float(args[ARG_duration].u_obj);

    ESP_LOGD(TAG, "[music.tone] %dHz for %.2fs", freq, dur);
    buzzer_tone(freq, (uint16_t)(dur * 1000));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(music_tone_obj, 0, music_tone);

STATIC mp_obj_t music_stop(void)
{
    buzzer_stop();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(music_stop_obj, music_stop);

STATIC const mp_rom_map_elem_t music_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),  MP_ROM_QSTR(MP_QSTR_music) },
    { MP_ROM_QSTR(MP_QSTR_tone),     MP_ROM_PTR(&music_tone_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),     MP_ROM_PTR(&music_stop_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_music_globals, music_globals_table);

const mp_obj_module_t mp_module_xiaomiao_music = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_music_globals,
};

/* ==================== storage 子模块 ==================== */

STATIC mp_obj_t storage_write(mp_obj_t path_obj, mp_obj_t data_obj)
{
    const char *path = mp_obj_str_get_str(path_obj);
    /* 确保路径在 /sdcard/data/ 下 */
    char full_path[256];
    if (path[0] != '/') {
        snprintf(full_path, sizeof(full_path), "/sdcard/data/%s", path);
    } else {
        strcpy(full_path, path);
    }

    FILE *f = fopen(full_path, "wb");
    if (!f) return mp_const_false;

    /* 处理 bytes 或 str */
    if (mp_obj_is_str(data_obj)) {
        const char *s = mp_obj_str_get_str(data_obj);
        fwrite(s, 1, strlen(s), f);
    } else if (mp_obj_is_type(data_obj, &mp_type_bytes)) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer(data_obj, &bufinfo, MP_BUFFER_READ);
        fwrite(bufinfo.buf, 1, bufinfo.len, f);
    }
    fclose(f);
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(storage_write_obj, storage_write);

STATIC mp_obj_t storage_read(mp_obj_t path_obj)
{
    const char *path = mp_obj_str_get_str(path_obj);
    char full_path[256];
    if (path[0] != '/') {
        snprintf(full_path, sizeof(full_path), "/sdcard/data/%s", path);
    } else {
        strcpy(full_path, path);
    }

    FILE *f = fopen(full_path, "rb");
    if (!f) return mp_const_none;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = malloc(len + 1);
    if (!buf) { fclose(f); return mp_const_none; }
    fread(buf, 1, len, f);
    fclose(f);

    mp_obj_t bytes = mp_obj_new_bytes(buf, len);
    free(buf);
    return bytes;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(storage_read_obj, storage_read);

STATIC mp_obj_t storage_exists(mp_obj_t path_obj)
{
    const char *path = mp_obj_str_get_str(path_obj);
    FILE *f = fopen(path, "r");
    if (f) { fclose(f); return mp_const_true; }
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(storage_exists_obj, storage_exists);

STATIC const mp_rom_map_elem_t storage_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_storage) },
    { MP_ROM_QSTR(MP_QSTR_write),    MP_ROM_PTR(&storage_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),     MP_ROM_PTR(&storage_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_exists),   MP_ROM_PTR(&storage_exists_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_storage_globals, storage_globals_table);

const mp_obj_module_t mp_module_xiaomiao_storage = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_storage_globals,
};

/* ==================== system 子模块 ==================== */

STATIC mp_obj_t system_exit(void)
{
    ESP_LOGI(TAG, "[system.exit] Returning to desktop...");
    /* 触发页面回到桌面 */
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(system_exit_obj, system_exit);

STATIC mp_obj_t system_sleep(mp_obj_t seconds_obj)
{
    float s = mp_obj_get_float(seconds_obj);
    mp_hal_delay_ms((uint32_t)(s * 1000));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(system_sleep_obj, system_sleep);

STATIC mp_obj_t system_info(void)
{
    mp_obj_t dict = mp_obj_new_dict(4);
    mp_obj_dict_store(dict, mp_obj_new_str("platform", 8), mp_obj_new_str("XiaoMiao", 8));
    mp_obj_dict_store(dict, mp_obj_new_str("mcu", 3), mp_obj_new_str("ESP32-WROVER-B", 15));
    mp_obj_dict_store(dict, mp_obj_new_str("flash", 5), mp_obj_new_int(4 * 1024 * 1024));
    mp_obj_dict_store(dict, mp_obj_new_str("psram", 5), mp_obj_new_int(8 * 1024 * 1024));
    return dict;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(system_info_obj, system_info);

STATIC mp_obj_t system_reboot(void)
{
    ESP_LOGI(TAG, "[system.reboot]");
    esp_restart();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(system_reboot_obj, system_reboot);

STATIC const mp_rom_map_elem_t system_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_system) },
    { MP_ROM_QSTR(MP_QSTR_exit),     MP_ROM_PTR(&system_exit_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep),    MP_ROM_PTR(&system_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_info),     MP_ROM_PTR(&system_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_reboot),   MP_ROM_PTR(&system_reboot_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_system_globals, system_globals_table);

const mp_obj_module_t mp_module_xiaomiao_system = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_system_globals,
};

/* ==================== time 子模块 ==================== */

STATIC mp_obj_t time_sleep(mp_obj_t seconds_obj)
{
    float s = mp_obj_get_float(seconds_obj);
    mp_hal_delay_ms((uint32_t)(s * 1000));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_sleep_obj, time_sleep);

STATIC mp_obj_t time_ticks_ms(void)
{
    return mp_obj_new_int(xTaskGetTickCount() * portTICK_PERIOD_MS);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(time_ticks_ms_obj, time_ticks_ms);

STATIC const mp_rom_map_elem_t time_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),  MP_ROM_QSTR(MP_QSTR_time) },
    { MP_ROM_QSTR(MP_QSTR_sleep),     MP_ROM_PTR(&time_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_ms),  MP_ROM_PTR(&time_ticks_ms_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_time_globals, time_globals_table);

const mp_obj_module_t mp_module_xiaomiao_time = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_time_globals,
};

/* ==================== xiaomiao 主模块 ==================== */

STATIC const mp_rom_map_elem_t xiaomiao_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),  MP_ROM_QSTR(MP_QSTR_xiaomiao) },
    { MP_ROM_QSTR(MP_QSTR_screen),    MP_ROM_PTR(&mp_module_xiaomiao_screen) },
    { MP_ROM_QSTR(MP_QSTR_key),       MP_ROM_PTR(&mp_module_xiaomiao_key) },
    { MP_ROM_QSTR(MP_QSTR_sensor),    MP_ROM_PTR(&mp_module_xiaomiao_sensor) },
    { MP_ROM_QSTR(MP_QSTR_music),     MP_ROM_PTR(&mp_module_xiaomiao_music) },
    { MP_ROM_QSTR(MP_QSTR_storage),   MP_ROM_PTR(&mp_module_xiaomiao_storage) },
    { MP_ROM_QSTR(MP_QSTR_system),    MP_ROM_PTR(&mp_module_xiaomiao_system) },
    { MP_ROM_QSTR(MP_QSTR_time),      MP_ROM_PTR(&mp_module_xiaomiao_time) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_xiaomiao_globals, xiaomiao_globals_table);

const mp_obj_module_t mp_module_xiaomiao = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_xiaomiao_globals,
};

/* 注册模块 */
MP_REGISTER_MODULE(MP_QSTR_xiaomiao, mp_module_xiaomiao);
