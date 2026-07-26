/*
 * editor.c - 积木编辑器主框架
 *
 * 屏幕布局 (160x128):
 *   ┌────────────────────────┐  ← 0
 *   │ 状态栏 (12px)           │
 *   ├────────────────────────┤  ← 12
 *   │┌────┐┌───────────────┐│
 *   ││分类││ 程序树预览      ││  ← 14
 *   ││    ││                ││
 *   ││    ││                ││
 *   │└────┘└───────────────┘│  ← 90
 *   │┌──────────────────────┐│
 *   ││ 积木列表/参数编辑     ││  ← 92
 *   │└──────────────────────┘│  ← 116
 *   │[A:插入 B:删 ←→分类↓↑积木]│  ← 116-128
 *   └────────────────────────┘
 */
#include <string.h>
#include "lvgl.h"
#include "block_editor/editor.h"
#include "block_editor/block_tree.h"
#include "block_editor/block_def.h"
#include "block_editor/renderer.h"
#include "block_editor/input.h"
#include "block_editor/codegen.h"
#include "ui/theme.h"
#include "ui/page_manager.h"
#include "ui/components.h"
#include "micropython/mp_engine.h"
#include "desktop/desktop.h"

static const char *TAG = "editor";

/* UI 对象 */
static lv_obj_t *s_editor_root = NULL;
static lv_obj_t *s_cat_list = NULL;     /* 左侧分类栏 */
static lv_obj_t *s_tree_area = NULL;    /* 程序树预览区 */
static lv_obj_t *s_bottom_area = NULL;  /* 底部积木列表/参数 */
static lv_obj_t *s_status_line = NULL;  /* 底部状态行 */
static lv_obj_t *s_param_edit_lbl = NULL; /* 参数编辑显示 */

static program_tree_t *s_tree = NULL;
static char s_current_file[128] = {0};

/* 分类栏高度 */
#define CAT_LIST_W  28
#define TREE_AREA_H 76
#define BOTTOM_H    24

/* 分类名（短） */
static const char *CAT_SHORT[] = {"Ev", "Ct", "Dp", "Ky", "Sn", "Sd"};

/* ---------- 内部函数 ---------- */
static void editor_refresh(void)
{
    if (!s_tree) return;

    edit_mode_t mode = editor_get_mode();

    /* 刷新分类栏 */
    if (s_cat_list) {
        lv_obj_clean(s_cat_list);
        for (int i = 0; i < BLOCK_CAT_COUNT; i++) {
            lv_obj_t *item = lv_obj_create(s_cat_list);
            lv_obj_set_size(item, CAT_LIST_W - 2, 14);
            lv_obj_set_pos(item, 0, i * 15);
            bool sel = (i == (int)s_cat);
            lv_obj_set_style_bg_color(item, sel ? lv_color_hex(0x444444) : THEME_BG_LIGHT, 0);
            lv_obj_set_style_radius(item, 0, 0);
            lv_obj_set_style_border_width(item, 0, 0);

            lv_obj_t *lbl = lv_label_create(item);
            lv_label_set_text(lbl, CAT_SHORT[i]);
            lv_obj_set_style_text_color(lbl, sel ? lv_color_white() : lv_color_hex(0x888888), 0);
            lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
            lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        }
    }

    /* 刷新程序树区 */
    if (s_tree_area) {
        renderer_draw_tree(s_tree_area, s_tree);
    }

    /* 刷新底部区域 */
    if (s_bottom_area) {
        lv_obj_clean(s_bottom_area);

        if (mode == EDIT_MODE_PARAM_EDIT && s_tree && s_tree->cursor) {
            /* 参数编辑模式 */
            const block_def_t *def = block_get_def(s_tree->cursor->type);
            if (def) {
                char buf[64];
                snprintf(buf, sizeof(buf), "%s: %s", def->params[0].name, s_param_buf);
                lv_obj_t *lbl = lv_label_create(s_bottom_area);
                lv_label_set_text(lbl, buf);
                lv_obj_set_style_text_color(lbl, THEME_ACCENT, 0);
                lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
                lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 2, 0);

                lv_obj_t *hint = lv_label_create(s_bottom_area);
                lv_label_set_text(hint, "A:确认 B:取消");
                lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
                lv_obj_set_style_text_font(hint, FONT_6X8, 0);
                lv_obj_align(hint, LV_ALIGN_RIGHT_MID, -2, 0);
            }
        } else if (mode == EDIT_MODE_BLOCK_SELECT) {
            renderer_draw_block_list(s_bottom_area, s_cat, s_block_idx);
        } else {
            /* 显示当前光标积木的参数提示 */
            if (s_tree && s_tree->cursor) {
                const block_def_t *def = block_get_def(s_tree->cursor->type);
                if (def && def->param_count > 0) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "A:编辑参数 B:删除");
                    lv_obj_t *lbl = lv_label_create(s_bottom_area);
                    lv_label_set_text(lbl, buf);
                    lv_obj_set_style_text_color(lbl, THEME_ACCENT, 0);
                    lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
                    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 2, 0);
                }
            }
        }
    }

    /* 刷新状态行 */
    if (s_status_line) {
        char buf[80];
        const char *mode_str = "?";
        switch (mode) {
            case EDIT_MODE_CAT_SELECT:  mode_str = "分类选择 ←→移动 ↓进入"; break;
            case EDIT_MODE_BLOCK_SELECT: mode_str = "积木选择 ↑↓选 A插入 B返回"; break;
            case EDIT_MODE_TREE_VIEW:   mode_str = "程序树 A:编辑 B:删除 运行:..."; break;
            case EDIT_MODE_PARAM_EDIT:  mode_str = "参数编辑"; break;
            case EDIT_MODE_RUN:         mode_str = "运行中... B停止"; break;
        }
        snprintf(buf, sizeof(buf), "[%s] 节点:%d", mode_str, s_tree ? s_tree->node_count : 0);
        lv_label_set_text(s_status_line, buf);
    }
}

static void editor_save_and_run(void)
{
    if (!s_tree) return;

    char code[4096];
    codegen_generate(s_tree, code, sizeof(code));

    /* 保存到临时文件 */
    FILE *f = fopen("/sdcard/data/_current_main.py", "w");
    if (f) {
        fwrite(code, 1, strlen(code), f);
        fclose(f);
    }

    /* 在 MicroPython 中执行 */
    mp_engine_exec(code);
}

/* ---------- 公开 API ---------- */
void block_editor_open(const char *block_file_path)
{
    ESP_LOGI(TAG, "Opening block editor...");

    lv_obj_t *scr = lv_screen_active();

    /* 根容器 */
    s_editor_root = lv_obj_create(scr);
    lv_obj_set_size(s_editor_root, SCREEN_W, SCREEN_H - STATUSBAR_H);
    lv_obj_set_pos(s_editor_root, 0, STATUSBAR_H);
    lv_obj_set_style_bg_color(s_editor_root, THEME_BG, 0);
    lv_obj_set_style_border_width(s_editor_root, 0, 0);
    lv_obj_set_style_pad_all(s_editor_root, 1, 0);

    /* 分类栏（左侧） */
    s_cat_list = lv_obj_create(s_editor_root);
    lv_obj_set_size(s_cat_list, CAT_LIST_W, TREE_AREA_H);
    lv_obj_set_pos(s_cat_list, 0, 1);
    lv_obj_set_style_bg_color(s_cat_list, THEME_BG, 0);
    lv_obj_set_style_border_width(s_cat_list, 0, 0);
    lv_obj_set_style_pad_all(s_cat_list, 0, 0);
    lv_obj_set_style_overflow(s_cat_list, LV_OVERFLOW_VISIBLE, 0);

    /* 程序树预览区 */
    s_tree_area = lv_obj_create(s_editor_root);
    lv_obj_set_size(s_tree_area, SCREEN_W - CAT_LIST_W - 2, TREE_AREA_H);
    lv_obj_set_pos(s_tree_area, CAT_LIST_W + 1, 1);
    lv_obj_set_style_bg_color(s_tree_area, THEME_BG, 0);
    lv_obj_set_style_border_color(s_tree_area, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(s_tree_area, 1, 0);
    lv_obj_set_style_pad_all(s_tree_area, 0, 0);
    lv_obj_set_style_overflow(s_tree_area, LV_OVERFLOW_VISIBLE, 0);

    /* 底部区域（积木列表/参数编辑） */
    s_bottom_area = lv_obj_create(s_editor_root);
    lv_obj_set_size(s_bottom_area, SCREEN_W - 2, BOTTOM_H);
    lv_obj_set_pos(s_bottom_area, 1, TREE_AREA_H + 2);
    lv_obj_set_style_bg_color(s_bottom_area, THEME_BG_LIGHT, 0);
    lv_obj_set_style_border_width(s_bottom_area, 0, 0);
    lv_obj_set_style_pad_all(s_bottom_area, 1, 0);
    lv_obj_set_style_overflow(s_bottom_area, LV_OVERFLOW_VISIBLE, 0);

    /* 状态行 */
    s_status_line = lv_label_create(s_editor_root);
    lv_label_set_text(s_status_line, "[分类选择]");
    lv_obj_set_style_text_color(s_status_line, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(s_status_line, FONT_6X8, 0);
    lv_obj_align(s_status_line, LV_ALIGN_BOTTOM_LEFT, 2, -1);

    /* 创建程序树 */
    if (!s_tree) {
        s_tree = program_tree_create();
    }

    /* 保存文件路径 */
    if (block_file_path) {
        strcpy(s_current_file, block_file_path);
    } else {
        strcpy(s_current_file, "/sdcard/programs/untitled.json");
    }

    /* 初始化输入 */
    editor_input_init(s_tree);

    /* 隐藏桌面 */
    lv_obj_add_flag(page_get(PAGE_DESKTOP), LV_OBJ_FLAG_HIDDEN);

    page_register(PAGE_BLOCK_EDITOR, s_editor_root);
    page_show(PAGE_BLOCK_EDITOR);

    editor_refresh();
    ESP_LOGI(TAG, "Block editor ready");
}

void block_editor_close(void)
{
    if (s_editor_root) {
        lv_obj_add_flag(s_editor_root, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_tree) {
        program_tree_free(s_tree);
        s_tree = NULL;
    }
    /* 显示桌面 */
    lv_obj_remove_flag(page_get(PAGE_DESKTOP), LV_OBJ_FLAG_HIDDEN);
    desktop_refresh();
}

void block_editor_handle_key(int key)
{
    if (!s_tree) return;

    edit_mode_t before = editor_get_mode();

    /* 全局快捷键 */
    if (key == 5 && before == EDIT_MODE_TREE_VIEW) {
        /* A 在树视图 → 运行程序 */
        editor_save_and_run();
        return;
    }

    /* 委托给 input 模块 */
    editor_input_handle_key(key);

    /* 如果模式变为 PARAM_EDIT，同步参数缓冲 */
    edit_mode_t after = editor_get_mode();
    if (after == EDIT_MODE_PARAM_EDIT && before != EDIT_MODE_PARAM_EDIT) {
        /* 已经开始编辑，无需额外操作 */
    }

    editor_refresh();
}

program_tree_t *block_editor_get_tree(void) { return s_tree; }
