#pragma once
#include "block_editor/block_def.h"

/* 程序树管理 */
typedef struct {
    block_node_t *root;       /* 根节点链表 */
    block_node_t *cursor;      /* 当前选中节点 */
    int           node_count;
} program_tree_t;

program_tree_t *program_tree_create(void);
void            program_tree_free(program_tree_t *tree);

/* 插入/删除 */
void program_tree_insert(program_tree_t *tree, block_type_t type);
void program_tree_delete_current(program_tree_t *tree);

/* 移动光标 */
void program_tree_move_up(program_tree_t *tree);
void program_tree_move_down(program_tree_t *tree);
void program_tree_indent_left(program_tree_t *tree);
void program_tree_indent_right(program_tree_t *tree);

/* 序列化/反序列化 JSON */
int  program_tree_to_json(program_tree_t *tree, char *buf, int buf_size);
bool program_tree_from_json(program_tree_t *tree, const char *json);

/* 生成代码 */
int  program_tree_generate_code(program_tree_t *tree, char *buf, int buf_size);
