#pragma once
/*
 * icons_data.h - 图标数据声明
 *
 * 使用 LVGL 内置符号字体作为图标源:
 *   LV_SYMBOL_OK, LV_SYMBOL_CLOSE, LV_SYMBOL_EDIT,
 *   LV_SYMBOL_TRASH, LV_SYMBOL_UP, LV_SYMBOL_DOWN, ...
 */

#include "lvgl.h"

/* 系统图标符号 */
#define ICON_HOME     LV_SYMBOL_HOME
#define ICON_SETTINGS LV_SYMBOL_SETTINGS
#define ICON_CLOSE    LV_SYMBOL_CLOSE
#define ICON_OK       LV_SYMBOL_OK
#define ICON_EDIT     LV_SYMBOL_EDIT
#define ICON_TRASH    LV_SYMBOL_TRASH
#define ICON_UP       LV_SYMBOL_UP
#define ICON_DOWN     LV_SYMBOL_DOWN
#define ICON_LEFT     LV_SYMBOL_LEFT
#define ICON_RIGHT    LV_SYMBOL_RIGHT
#define ICON_PLAY     LV_SYMBOL_PLAY
#define ICON_PAUSE    LV_SYMBOL_PAUSE
#define ICON_STOP     LV_SYMBOL_STOP
#define ICON_BATTERY  LV_SYMBOL_BATTERY_FULL
#define ICON_WIFI     LV_SYMBOL_WIFI

/* 应用图标符号映射 */
typedef enum {
    APP_ICON_CODE   = 0,
    APP_ICON_GEAR   = 1,
    APP_ICON_FILE   = 2,
    APP_ICON_INFO   = 3,
    APP_ICON_GAME   = 4,
    APP_ICON_SHAKE  = 5,
    APP_ICON_POWER  = 6,
    APP_ICON_MUSIC  = 7,
} app_icon_id_t;

/* 获取图标符号字符串 */
const char *ui_get_icon_symbol(app_icon_id_t id);
