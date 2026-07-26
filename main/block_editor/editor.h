#pragma once
#include "lvgl.h"
#include "block_editor/block_tree.h"

/* 打开积木编辑器 */
void block_editor_open(const char *block_file_path);

/* 关闭编辑器 */
void block_editor_close(void);

/* 处理按键 */
void block_editor_handle_key(int key);

/* 获取当前程序树（供外部保存/运行） */
program_tree_t *block_editor_get_tree(void);
