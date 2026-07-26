#pragma once
#include "lvgl.h"

/* 桌面应用项 */
typedef struct {
    char     name[32];      /* 显示名称 */
    char     package[64];   /* 包名 (com.xxx.yyy) */
    char     icon_path[128]; /* 图标路径 (SD) */
    char     entry_path[128];/* 入口脚本路径 */
    lv_color_t tile_color;  /* 磁贴颜色 */
    bool     is_system;     /* 系统内置 */
    bool     is_block_app;  /* 是否积木程序 */
    char     block_file[128];/* 积木 JSON 文件路径 */
} app_item_t;

#define MAX_APPS 32
extern app_item_t g_apps[MAX_APPS];
extern int        g_app_count;

void desktop_create(void);
void desktop_refresh(void);
void desktop_handle_key(int key);
