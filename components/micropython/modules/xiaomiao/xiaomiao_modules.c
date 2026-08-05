/*
 * XiaoMiao Hardware API Module for MicroPython
 * 
 * This module provides Python bindings for XiaoMiao hardware:
 * - lcd: Display control (ST7735 160x128)
 * - buttons: 6-key input (UP/DOWN/LEFT/RIGHT/A/B)
 * - buzzer: Sound generation
 * - power: Battery status
 * - time: System time
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"

static const char *TAG = "xiaomiao";

/* ── LCD Module ─────────────────────────────────────────────────────────── */

STATIC mp_obj_t xiaomiao_lcd_clear(mp_obj_t color_obj) {
    mp_int_t color = mp_obj_get_int(color_obj);
    ESP_LOGI(TAG, "LCD clear: color=0x%04X", (uint16_t)color);
    // TODO: Call actual LCD clear function
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(xiaomiao_lcd_clear_obj, xiaomiao_lcd_clear);

STATIC mp_obj_t xiaomiao_lcd_text(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t text_obj, mp_obj_t color_obj) {
    mp_int_t x = mp_obj_get_int(x_obj);
    mp_int_t y = mp_obj_get_int(y_obj);
    const char *text = mp_obj_str_get_str(text_obj);
    mp_int_t color = mp_obj_get_int(color_obj);
    ESP_LOGI(TAG, "LCD text: (%d,%d) '%s' color=0x%04X", x, y, text, (uint16_t)color);
    // TODO: Call actual LCD text rendering
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_4(xiaomiao_lcd_text_obj, xiaomiao_lcd_text);

STATIC mp_obj_t xiaomiao_lcd_rect(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t w_obj, mp_obj_t h_obj, mp_obj_t color_obj) {
    mp_int_t x = mp_obj_get_int(x_obj);
    mp_int_t y = mp_obj_get_int(y_obj);
    mp_int_t w = mp_obj_get_int(w_obj);
    mp_int_t h = mp_obj_get_int(h_obj);
    mp_int_t color = mp_obj_get_int(color_obj);
    ESP_LOGI(TAG, "LCD rect: (%d,%d) %dx%d color=0x%04X", x, y, w, h, (uint16_t)color);
    // TODO: Call actual LCD rectangle drawing
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_5(xiaomiao_lcd_rect_obj, xiaomiao_lcd_rect);

STATIC mp_obj_t xiaomiao_lcd_fill_rect(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t w_obj, mp_obj_t h_obj, mp_obj_t color_obj) {
    mp_int_t x = mp_obj_get_int(x_obj);
    mp_int_t y = mp_obj_get_int(y_obj);
    mp_int_t w = mp_obj_get_int(w_obj);
    mp_int_t h = mp_obj_get_int(h_obj);
    mp_int_t color = mp_obj_get_int(color_obj);
    ESP_LOGI(TAG, "LCD fill_rect: (%d,%d) %dx%d color=0x%04X", x, y, w, h, (uint16_t)color);
    // TODO: Call actual LCD filled rectangle drawing
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_5(xiaomiao_lcd_fill_rect_obj, xiaomiao_lcd_fill_rect);

STATIC const mp_rom_map_elem_t xiaomiao_lcd_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lcd) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&xiaomiao_lcd_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&xiaomiao_lcd_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&xiaomiao_lcd_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&xiaomiao_lcd_fill_rect_obj) },
};
STATIC MP_DEFINE_CONST_DICT(xiaomiao_lcd_module_globals, xiaomiao_lcd_module_globals_table);

const mp_obj_module_t xiaomiao_lcd_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&xiaomiao_lcd_module_globals,
};

/* ── Buttons Module ─────────────────────────────────────────────────────── */

STATIC mp_obj_t xiaomiao_buttons_read(void) {
    // TODO: Read actual button states
    // Return dict with UP/DOWN/LEFT/RIGHT/A/B keys
    mp_obj_t dict = mp_obj_new_dict(6);
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_UP), mp_obj_new_bool(false));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_DOWN), mp_obj_new_bool(false));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_LEFT), mp_obj_new_bool(false));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_RIGHT), mp_obj_new_bool(false));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_A), mp_obj_new_bool(false));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_B), mp_obj_new_bool(false));
    return dict;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(xiaomiao_buttons_read_obj, xiaomiao_buttons_read);

STATIC mp_obj_t xiaomiao_buttons_wait(void) {
    ESP_LOGI(TAG, "Waiting for button press...");
    // TODO: Block until button press
    mp_hal_delay_ms(100);
    return MP_OBJ_NEW_QSTR(MP_QSTR_A);  // Placeholder
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(xiaomiao_buttons_wait_obj, xiaomiao_buttons_wait);

STATIC const mp_rom_map_elem_t xiaomiao_buttons_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_buttons) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&xiaomiao_buttons_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_wait), MP_ROM_PTR(&xiaomiao_buttons_wait_obj) },
};
STATIC MP_DEFINE_CONST_DICT(xiaomiao_buttons_module_globals, xiaomiao_buttons_module_globals_table);

const mp_obj_module_t xiaomiao_buttons_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&xiaomiao_buttons_module_globals,
};

/* ── Buzzer Module ──────────────────────────────────────────────────────── */

STATIC mp_obj_t xiaomiao_buzzer_beep(mp_obj_t freq_obj, mp_obj_t duration_obj) {
    mp_int_t freq = mp_obj_get_int(freq_obj);
    mp_int_t duration = mp_obj_get_int(duration_obj);
    ESP_LOGI(TAG, "Buzzer beep: %dHz for %dms", freq, duration);
    // TODO: Call actual buzzer control
    mp_hal_delay_ms(duration);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(xiaomiao_buzzer_beep_obj, xiaomiao_buzzer_beep);

STATIC const mp_rom_map_elem_t xiaomiao_buzzer_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_buzzer) },
    { MP_ROM_QSTR(MP_QSTR_beep), MP_ROM_PTR(&xiaomiao_buzzer_beep_obj) },
};
STATIC MP_DEFINE_CONST_DICT(xiaomiao_buzzer_module_globals, xiaomiao_buzzer_module_globals_table);

const mp_obj_module_t xiaomiao_buzzer_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&xiaomiao_buzzer_module_globals,
};

/* ── Power Module ───────────────────────────────────────────────────────── */

STATIC mp_obj_t xiaomiao_power_voltage(void) {
    // TODO: Read actual battery voltage
    return mp_obj_new_float(3.7f);  // Placeholder
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(xiaomiao_power_voltage_obj, xiaomiao_power_voltage);

STATIC mp_obj_t xiaomiao_power_percent(void) {
    // TODO: Read actual battery percentage
    return mp_obj_new_int(75);  // Placeholder
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(xiaomiao_power_percent_obj, xiaomiao_power_percent);

STATIC const mp_rom_map_elem_t xiaomiao_power_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_power) },
    { MP_ROM_QSTR(MP_QSTR_voltage), MP_ROM_PTR(&xiaomiao_power_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_percent), MP_ROM_PTR(&xiaomiao_power_percent_obj) },
};
STATIC MP_DEFINE_CONST_DICT(xiaomiao_power_module_globals, xiaomiao_power_module_globals_table);

const mp_obj_module_t xiaomiao_power_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&xiaomiao_power_module_globals,
};

/* ── Time Module ────────────────────────────────────────────────────────── */

STATIC mp_obj_t xiaomiao_time_ms(void) {
    int64_t us = esp_timer_get_time();
    return mp_obj_new_int(us / 1000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(xiaomiao_time_ms_obj, xiaomiao_time_ms);

STATIC mp_obj_t xiaomiao_time_sleep(mp_obj_t ms_obj) {
    mp_int_t ms = mp_obj_get_int(ms_obj);
    mp_hal_delay_ms(ms);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(xiaomiao_time_sleep_obj, xiaomiao_time_sleep);

STATIC const mp_rom_map_elem_t xiaomiao_time_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_time) },
    { MP_ROM_QSTR(MP_QSTR_ms), MP_ROM_PTR(&xiaomiao_time_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&xiaomiao_time_sleep_obj) },
};
STATIC MP_DEFINE_CONST_DICT(xiaomiao_time_module_globals, xiaomiao_time_module_globals_table);

const mp_obj_module_t xiaomiao_time_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&xiaomiao_time_module_globals,
};

/* ── Module Registration ────────────────────────────────────────────────── */

void xiaomiao_modules_register(void) {
    ESP_LOGI(TAG, "Registering XiaoMiao hardware modules");
    
    // Register modules to be accessible from Python
    // These will be available as: import lcd, import buttons, etc.
    // TODO: Integrate with MicroPython module registration system
}