/*
 * launcher.c - 应用列表/启动器
 *
 * 显示所有已安装的应用（系统 + 用户）
 * 上下选择，A 启动，B 返回桌面
 */
#include "lvgl.h"
#include "desktop/launcher.h"
#include "desktop/desktop.h"
#include "desktop/statusbar.h"
#include "ui/theme.h"
#include "ui/components.h"
#include "ui/page_manager.h"
#include "app_runtime/app_manager.h"
#include "block_editor/editor.h"

static const char *TAG = "launcher";

static lv_obj_t *s_launcher_root = NULL;
static int       s_cursor = 0;
static int       s_scroll = 0;
#define ITEMS_PER_PAGE 5

static const char *SYS_NAMES[] = {"编程", "设置", "文件", "关于", "贪吃蛇", "摇一摇", "电池", "音乐"};
static const lv_color_t SYS_COLORS[] = {
    THEME_TILE_3, THEME_TILE_7, THEME_TILE_5, THEME_TILE_6,
    THEME_TILE_2, THEME_TILE_4, THEME_TILE_1, THEME_TILE_8,
};

void launcher_open(void)
{
    lv_obj_t *scr = lv_screen_active();

    s_launcher_root = lv_obj_create(scr);
    lv_obj_set_size(s_launcher_root, SCREEN_W, SCREEN_H - STATUSBAR_H);
    lv_obj_set_pos(s_launcher_root, 0, STATUSBAR_H);
    lv_obj_set_style_bg_color(s_launcher_root, THEME_BG, 0);
    lv_obj_set_style_border_width(s_launcher_root, 0, 0);
    lv_obj_set_style_pad_all(s_launcher_root, 2, 0);

    /* 标题 */
    lv_obj_t *title = lv_label_create(s_launcher_root);
    lv_label_set_text(title, "应用列表");
    lv_obj_set_style_text_color(title, THEME_ACCENT, 0);
    lv_obj_set_style_text_font(title, FONT_8X8, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 2);

    s_cursor = 0;
    s_scroll = 0;

    page_register(PAGE_APP_LIST, s_launcher_root);
    page_show(PAGE_APP_LIST);
    lv_obj_add_flag(page_get(PAGE_DESKTOP), LV_OBJ_FLAG_HIDDEN);

    launcher_handle_key(0); /* 触发重绘 */
    ESP_LOGI(TAG, "Launcher opened");
}

static void launcher_redraw(void)
{
    if (!s_launcher_root) return;

    /* 清除旧内容（保留标题） */
    lv_obj_clean(s_launcher_root);

    /* 标题 */
    lv_obj_t *title = lv_label_create(s_launcher_root);
    lv_label_set_text(title, "应用列表");
    lv_obj_set_style_text_color(title, THEME_ACCENT, 0);
    lv_obj_set_style_text_font(title, FONT_8X8, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 2);

    int total = g_app_count + 8; /* 8 系统应用 */
    int y = 16;

    for (int i = s_scroll; i < total && (i - s_scroll) < ITEMS_PER_PAGE; i++) {
        bool selected = (i == s_cursor);
        const char *name;
        lv_color_t color;

        if (i < 8) {
            name = SYS_NAMES[i];
            color = SYS_COLORS[i];
        } else {
            name = g_apps[i - 8].name;
            color = g_apps[i - 8].tile_color;
        }

        lv_obj_t *item = lv_obj_create(s_launcher_root);
        lv_obj_set_size(item, SCREEN_W - 6, 18);
        lv_obj_set_pos(item, 2, y);
        lv_obj_set_style_bg_color(item, selected ? color : THEME_BG_LIGHT, 0);
        lv_obj_set_style_radius(item, 0, 0);
        lv_obj_set_style_border_width(item, 0, 0);

        /* 色条 */
        lv_obj_t *bar = lv_obj_create(item);
        lv_obj_set_size(bar, 3, 14);
        lv_obj_set_pos(bar, 1, 2);
        lv_obj_set_style_bg_color(bar, color, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_set_style_border_width(bar, 0, 0);

        /* 名称 */
        lv_obj_t *lbl = lv_label_create(item);
        lv_label_set_text(lbl, name);
        lv_obj_set_style_text_color(lbl, selected ? lv_color_white() : THEME_FG, 0);
        lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 6, 0);

        /* 类型标签 */
        lv_obj_t *tag = lv_label_create(item);
        lv_label_set_text(tag, i < 8 ? "[SYS]" : "[APP]");
        lv_obj_set_style_text_color(tag, lv_color_hex(0x666666), 0);
        lv_obj_set_style_text_font(tag, FONT_6X8, 0);
        lv_obj_align(tag, LV_ALIGN_RIGHT_MID, -4, 0);

        y += 19;
    }

    /* 底部提示 */
    lv_obj_t *hint = lv_label_create(s_launcher_root);
    lv_label_set_text(hint, "A:打开 B:返回");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, FONT_6X8, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -2);
}

void launcher_handle_key(int key)
{
    int total = g_app_count + 8;

    switch (key) {
        case 1: /* UP */
            if (s_cursor > 0) {
                s_cursor--;
                if (s_cursor < s_scroll) s_scroll = s_cursor;
            }
            break;
        case 2: /* DOWN */
            if (s_cursor < total - 1) {
                s_cursor++;
                if (s_cursor >= s_scroll + ITEMS_PER_PAGE) s_scroll = s_cursor - ITEMS_PER_PAGE + 1;
            }
            break;
        case 5: /* A - 启动 */
            if (s_cursor < 8) {
                /* 系统应用 */
                switch (s_cursor) {
                    case 0: /* 编程 */
                        block_editor_open(NULL);
                        return;
                    case 1: settings_open(); return;
                    case 2: ui_alert("文件", "功能开发中"); break;
                    case 3: ui_alert("XiaoMiao OS", "v1.0.0\nESP32-WROVER-B\n160x128 ST7735"); break;
                    default: ui_alert("提示", "功能开发中"); break;
                }
            } else {
                int idx = s_cursor - 8;
                if (idx < g_app_count) {
                    if (g_apps[idx].is_block_app) {
                        block_editor_open(g_apps[idx].block_file);
                    } else {
                        app_runtime_launch_app(&g_apps[idx]);
                    }
                }
            }
            break;
        case 6: /* B - 返回桌面 */
            lv_obj_add_flag(s_launcher_root, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(page_get(PAGE_DESKTOP), LV_OBJ_FLAG_HIDDEN);
            desktop_refresh();
            return;
    }

    launcher_redraw();
}

void launcher_install_app(const char *path)
{
    ESP_LOGI(TAG, "Installing: %s", path);
    app_manager_install(path);
    desktop_refresh();
}

void launcher_uninstall_app(int index)
{
    if (index >= 0 && index < g_app_count) {
        app_manager_uninstall(g_apps[index].package);
        desktop_refresh();
    }
}
