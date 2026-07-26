/*
 * desktop.c - 桌面主界面
 *
 * 布局 (160x128):
 *   [状态栏 0-11]
 *   [磁贴区 12-113]  4列×2行, 每格 38x50 + 间隙
 *   [导航栏 114-127]  < 返回 | 桌面 | 应用列表 >
 *
 * 导航: 方向键移动光标, A=打开, B=无操作
 */
#include "lvgl.h"
#include "desktop/desktop.h"
#include "desktop/statusbar.h"
#include "ui/theme.h"
#include "ui/components.h"
#include "ui/page_manager.h"
#include "app_runtime/app_manager.h"
#include "block_editor/editor.h"

static const char *TAG = "desktop";

/* 全局应用列表 */
app_item_t g_apps[MAX_APPS];
int        g_app_count = 0;

/* UI 对象 */
static lv_obj_t *s_desktop_root = NULL;
static lv_obj_t *s_tiles[8] = {NULL};  /* 4×2 = 8 个磁贴 */
static int       s_cursor = 0;          /* 当前光标位置 0-7 */
static int       s_page_offset = 0;     /* 应用列表分页 */

/* 系统内置应用 */
static const char *SYS_APPS[] = {
    "编程", "设置", "文件", "关于",
    "贪吃蛇", "摇一摇", "电池", "音乐",
};
static const lv_color_t SYS_COLORS[] = {
    THEME_TILE_3, THEME_TILE_7, THEME_TILE_5, THEME_TILE_6,
    THEME_TILE_2, THEME_TILE_4, THEME_TILE_1, THEME_TILE_8,
};

#define SYS_APP_COUNT 8

/* ---------- 内部函数 ---------- */
static void desktop_draw_tiles(void)
{
    int start_idx = s_page_offset * 8;
    int total = g_app_count + SYS_APP_COUNT;

    for (int i = 0; i < 8; i++) {
        int app_idx = start_idx + i;
        lv_obj_t *tile = s_tiles[i];

        if (app_idx >= total) {
            lv_obj_add_flag(tile, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_remove_flag(tile, LV_OBJ_FLAG_HIDDEN);

        /* 获取标签和颜色 */
        const char *label;
        lv_color_t color;

        if (app_idx < SYS_APP_COUNT) {
            label = SYS_APPS[app_idx];
            color = SYS_COLORS[app_idx];
        } else {
            int u = app_idx - SYS_APP_COUNT;
            label = g_apps[u].name;
            color = g_apps[u].tile_color;
        }

        /* 更新磁贴 */
        lv_obj_set_style_bg_color(tile, color, 0);

        /* 清除旧子对象 */
        lv_obj_clean(tile);

        /* 添加标签 */
        lv_obj_t *lbl = lv_label_create(tile);
        lv_label_set_text(lbl, label);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
}

static void desktop_update_cursor(void)
{
    for (int i = 0; i < 8; i++) {
        if (s_tiles[i] && !lv_obj_has_flag(s_tiles[i], LV_OBJ_FLAG_HIDDEN)) {
            if (i == s_cursor) {
                lv_obj_set_style_border_width(s_tiles[i], 2, 0);
                lv_obj_set_style_border_color(s_tiles[i], lv_color_white(), 0);
            } else {
                lv_obj_set_style_border_width(s_tiles[i], 0, 0);
            }
        }
    }
}

static void desktop_launch(int app_idx)
{
    if (app_idx < SYS_APP_COUNT) {
        /* 系统应用 */
        switch (app_idx) {
            case 0: /* 编程 → 积木编辑器 */
                block_editor_open(NULL);
                break;
            case 1: /* 设置 */
                settings_open();
                break;
            case 2: /* 文件 */
                /* TODO: 文件管理器 */
                ui_alert("文件管理", "功能开发中");
                break;
            case 3: /* 关于 */
                ui_alert("XiaoMiao OS", "v1.0.0\nESP32-WROVER-B\n160x128 ST7735");
                break;
            case 4: /* 贪吃蛇 */
                app_runtime_launch_system("snake");
                break;
            case 5: /* 摇一摇 */
                app_runtime_launch_system("shake");
                break;
            case 6: /* 电池 */
                app_runtime_launch_system("battery_monitor");
                break;
            case 7: /* 音乐 */
                app_runtime_launch_system("music");
                break;
        }
    } else {
        /* 用户安装的应用 */
        int u = app_idx - SYS_APP_COUNT;
        if (u < g_app_count) {
            if (g_apps[u].is_block_app) {
                /* 积木程序 → 打开编辑器 */
                block_editor_open(g_apps[u].block_file);
            } else {
                /* MicroPython App → 直接运行 */
                app_runtime_launch_app(&g_apps[u]);
            }
        }
    }
}

/* ---------- 公开 API ---------- */
void desktop_create(void)
{
    ESP_LOGI(TAG, "Creating desktop...");

    lv_obj_t *scr = lv_screen_active();

    /* 根容器 */
    s_desktop_root = lv_obj_create(scr);
    lv_obj_set_size(s_desktop_root, SCREEN_W, DESKTOP_H);
    lv_obj_set_pos(s_desktop_root, 0, STATUSBAR_H);
    lv_obj_set_style_bg_color(s_desktop_root, THEME_BG, 0);
    lv_obj_set_style_border_width(s_desktop_root, 0, 0);
    lv_obj_set_style_pad_all(s_desktop_root, 2, 0);

    /* 创建 4×2 磁贴 */
    int tile_w = (SCREEN_W - 10) / 4;  /* 37 */
    int tile_h = (DESKTOP_H - 6) / 2;  /* 48 */
    int gap = 2;

    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 4; col++) {
            int idx = row * 4 + col;
            int x = 2 + col * (tile_w + gap);
            int y = 2 + row * (tile_h + gap);

            lv_obj_t *tile = lv_obj_create(s_desktop_root);
            lv_obj_set_size(tile, tile_w, tile_h);
            lv_obj_set_pos(tile, x, y);
            lv_obj_set_style_radius(tile, 0, 0);
            lv_obj_set_style_border_width(tile, 0, 0);
            lv_obj_set_style_pad_all(tile, 2, 0);
            lv_obj_set_style_bg_color(tile, THEME_TILE_1, 0);

            s_tiles[idx] = tile;
        }
    }

    /* 扫描已安装应用 */
    app_manager_scan();

    /* 绘制 */
    desktop_refresh();
    desktop_update_cursor();

    /* 注册到页面管理器 */
    page_register(PAGE_DESKTOP, s_desktop_root);

    ESP_LOGI(TAG, "Desktop ready. %d user apps installed.", g_app_count);
}

void desktop_refresh(void)
{
    desktop_draw_tiles();
    desktop_update_cursor();
}

void desktop_handle_key(int key)
{
    int total = g_app_count + SYS_APP_COUNT;
    int max_page = (total + 7) / 8;
    int cursor_row = s_cursor / 4;
    int cursor_col = s_cursor % 4;

    switch (key) {
        case 1: /* UP */
            if (cursor_row > 0) {
                s_cursor -= 4;
            } else {
                /* 上翻页 */
                if (s_page_offset > 0) {
                    s_page_offset--;
                    desktop_draw_tiles();
                }
            }
            desktop_update_cursor();
            break;

        case 2: /* DOWN */
            if (cursor_row == 0 && (s_cursor + 4) < total) {
                s_cursor += 4;
            } else if (s_page_offset < max_page - 1) {
                s_page_offset++;
                desktop_draw_tiles();
            }
            desktop_update_cursor();
            break;

        case 3: /* LEFT */
            if (cursor_col > 0) s_cursor--;
            else if (s_page_offset > 0) {
                s_page_offset--;
                s_cursor = 3;
                desktop_draw_tiles();
            }
            desktop_update_cursor();
            break;

        case 4: /* RIGHT */
            if (cursor_col < 3 && (s_cursor + 1) < total) s_cursor++;
            else if (s_page_offset < max_page - 1) {
                s_page_offset++;
                s_cursor = 0;
                desktop_draw_tiles();
            }
            desktop_update_cursor();
            break;

        case 5: /* A - 启动 */
            desktop_launch(s_cursor + s_page_offset * 8);
            break;

        case 6: /* B - 无操作（在桌面） */
            break;
    }
}
