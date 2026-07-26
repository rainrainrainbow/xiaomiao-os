#pragma once
#include "block_editor/block_tree.h"

/* 编辑器模式 */
typedef enum {
    EDIT_MODE_CAT_SELECT,    /* 选择分类 */
    EDIT_MODE_BLOCK_SELECT,  /* 选择积木 */
    EDIT_MODE_PARAM_EDIT,    /* 编辑参数 */
    EDIT_MODE_TREE_VIEW,     /* 查看/移动程序树 */
    EDIT_MODE_RUN,           /* 运行程序 */
} edit_mode_t;

/* 初始化/退出 */
void editor_input_init(program_tree_t *tree);
void editor_input_handle_key(int key);

/* 获取当前模式 */
edit_mode_t editor_get_mode(void);

/* 参数编辑 */
void editor_start_param_edit(int param_idx);
void editor_param_input_char(char c);
void editor_param_backspace(void);
void editor_param_confirm(void);
