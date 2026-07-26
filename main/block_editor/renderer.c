/*
 * renderer.c - 积木树屏幕渲染
 *
 * 在 160x128 屏幕上渲染积木程序树:
 *   - 每行显示一块积木（缩进表示嵌套）
 *   - 光标行高亮
 *   - 不同分类不同颜色
 */
#include <string.h>
#include "lvgl.h"
#include "block_editor/renderer.h"
#include "block_editor/block_tree.h"
#include "block_editor/block_def.h"
#include "ui/theme.h"

/* 分类颜色 */
static const lv_color_t CAT_COLORS[BLOCK_CAT_COUNT] = {
    lv_color_hex(0xFFC107),  /* 事件 - 黄 */
    lv_color_hex(0x9C27B0),  /* 控制 - 紫 */
    lv_color_hex(0x2196F3),  /* 显示 - 蓝 */
    lv_color_hex(0x4CAF50),  /* 按键 - 绿 */
    lv_color_hex(0xFF5722),  /* 传感器 - 橙 */
    lv_color_hex(0xE91E63),  /* 声音 - 粉 */
};

/* 每行高度 */
#define ROW_H 12
#define MAX_VISIBLE_ROWS 6

/* 获取积木的显示文本 */
void renderer_get_block_label(block_type_t type, char *buf, int buf_size)
{
    const block_def_t *def = block_get_def(type);
    if (!def) { snprintf(buf, buf_size, "?"); return; }

    if (def->param_count == 0) {
        snprintf(buf, buf_size, "%s", def->name);
    } else if (def->param_count == 1) {
        snprintf(buf, buf_size, "%s %s", def->name, def->params[0].default_val);
    } else {
        snprintf(buf, buf_size, "%s ...", def->name);
    }
}

/* 渲染单条积木行 */
static void draw_block_row(lv_obj_t *parent, int y, block_node_t *node, bool selected)
{
    const block_def_t *def = block_get_def(node->type);
    if (!def) return;

    int x = 2 + node->indent * 6;  /* 缩进 */

    /* 背景条 */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, SCREEN_W - 4, ROW_H - 1);
    lv_obj_set_pos(row, 0, y);
    lv_color_t bg = selected ? CAT_COLORS[def->cat] : THEME_BG_LIGHT;
    lv_obj_set_style_bg_color(row, bg, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    /* 分类色条（左侧 2px） */
    lv_obj_t *cat_bar = lv_obj_create(row);
    lv_obj_set_size(cat_bar, 2, ROW_H - 3);
    lv_obj_set_pos(cat_bar, 0, 1);
    lv_obj_set_style_bg_color(cat_bar, CAT_COLORS[def->cat], 0);
    lv_obj_set_style_radius(cat_bar, 0, 0);
    lv_obj_set_style_border_width(cat_bar, 0, 0);

    /* 文本 */
    char label[64];
    if (def->param_count == 0) {
        snprintf(label, sizeof(label), "%s", def->name);
    } else if (def->param_count == 1) {
        snprintf(label, sizeof(label), "%s %s", def->name, node->params[0]);
    } else if (def->param_count == 2) {
        snprintf(label, sizeof(label), "%s %s,%s", def->name, node->params[0], node->params[1]);
    } else {
        snprintf(label, sizeof(label), "%s(%s,%s,%s,%s)", def->name,
                 node->params[0], node->params[1], node->params[2], node->params[3]);
    }

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, selected ? lv_color_black() : THEME_FG, 0);
    lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 5, 0);
}

/* 渲染整个程序树 */
void renderer_draw_tree(lv_obj_t *parent, program_tree_t *tree)
{
    /* 清空容器 */
    lv_obj_clean(parent);

    if (!tree || !tree->root) {
        lv_obj_t *empty = lv_label_create(parent);
        lv_label_set_text(empty, "(空程序)\n按A插入积木");
        lv_obj_set_style_text_color(empty, lv_color_hex(0x666666), 0);
        lv_obj_set_style_text_font(empty, FONT_6X8, 0);
        lv_obj_align(empty, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* 遍历树，渲染可见节点 */
    int y = 0;
    block_node_t *cur = tree->root;
    int idx = 0;
    while (cur && idx < MAX_VISIBLE_ROWS) {
        bool selected = (cur == tree->cursor);
        draw_block_row(parent, y, cur, selected);
        y += ROW_H;
        cur = cur->next;
        idx++;
    }
}

/* 渲染积木选择列表 */
void renderer_draw_block_list(lv_obj_t *parent, block_cat_t cat, int selected)
{
    lv_obj_clean(parent);

    int count = block_count_in_cat(cat);
    int y = 0;

    for (int i = 0; i < count; i++) {
        const block_def_t *def = block_get_by_index(cat, i);
        if (!def) continue;

        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_set_size(row, SCREEN_W - 4, ROW_H - 1);
        lv_obj_set_pos(row, 0, y);

        bool is_sel = (i == selected);
        lv_obj_set_style_bg_color(row, is_sel ? CAT_COLORS[cat] : THEME_BG_LIGHT, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_border_width(row, 0, 0);

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, def->name);
        lv_obj_set_style_text_color(lbl, is_sel ? lv_color_black() : THEME_FG, 0);
        lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);

        y += ROW_H;
    }
}
