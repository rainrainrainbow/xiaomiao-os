#pragma once
#include "block_editor/block_tree.h"

/* 代码生成器 - 高级接口 */
int  codegen_generate(program_tree_t *tree, char *buf, int buf_size);
void codegen_save_to_file(program_tree_t *tree, const char *filepath);
int  codegen_compile_check(const char *code, char *error_buf, int error_size);
