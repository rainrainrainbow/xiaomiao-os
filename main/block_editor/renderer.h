#pragma once
#include "lvgl.h"
#include "block_editor/block_tree.h"

/* 渲染程序树到 LVGL 容器 */
void renderer_draw_tree(lv_obj_t *parent, program_tree_t *tree);

/* 渲染积木列表（分类下的可选积木） */
void renderer_draw_block_list(lv_obj_t *parent, block_cat_t cat, int selected);

/* 获取积木显示文本（带参数替换） */
void renderer_get_block_label(block_type_t type, char *buf, int buf_size);
