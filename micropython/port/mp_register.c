/*
 * mp_register.c - 完整 xiaomiao 模块注册
 *
 * 此文件在 WITH_MICROPYTHON 宏启用时提供完整的
 * xiaomiao.screen/key/sensor/music/storage/system/time 模块注册。
 *
 * 对应的实现见 mp_module_xiaomiao_full.c
 */
#include "py/objmodule.h"
#include "py/qstr.h"
#include "py/runtime.h"

/* 子模块对象声明（在 mp_module_xiaomiao_full.c 中定义） */
extern const mp_obj_module_t mp_module_xiaomiao_screen;
extern const mp_obj_module_t mp_module_xiaomiao_key;
extern const mp_obj_module_t mp_module_xiaomiao_sensor;
extern const mp_obj_module_t mp_module_xiaomiao_music;
extern const mp_obj_module_t mp_module_xiaomiao_storage;
extern const mp_obj_module_t mp_module_xiaomiao_system;
extern const mp_obj_module_t mp_module_xiaomiao_time;

/* 主模块全局字典 */
extern const mp_obj_dict_t mp_module_xiaomiao_globals;

void mp_register_xiaomiao_modules(void)
{
    /* 在 MicroPython 中注册 xiaomiao 模块 */
    mp_store_global(mp_load_name(MP_QSTR_xiaomiao),
                    (mp_obj_t)&mp_module_xiaomiao);
}

/* 模块初始化钩子 - 在 mp_init() 中调用 */
void mp_xiaomiao_init(void)
{
    mp_register_xiaomiao_modules();
}
