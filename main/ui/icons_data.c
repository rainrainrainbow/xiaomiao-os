/*
 * icons_data.c - 图标符号映射
 */
#include "lvgl.h"
#include "ui/icons_data.h"

const char *ui_get_icon_symbol(app_icon_id_t id)
{
    switch (id) {
        case APP_ICON_CODE:  return LV_SYMBOL_EDIT;
        case APP_ICON_GEAR:  return LV_SYMBOL_SETTINGS;
        case APP_ICON_FILE:  return LV_SYMBOL_LIST;
        case APP_ICON_INFO:  return LV_SYMBOL_INFO;
        case APP_ICON_GAME:  return LV_SYMBOL_PLAY;
        case APP_ICON_SHAKE: return LV_SYMBOL_REFRESH;
        case APP_ICON_POWER: return LV_SYMBOL_POWER;
        case APP_ICON_MUSIC: return LV_SYMBOL_AUDIO;
        default: return LV_SYMBOL_DUMMY;
    }
}
