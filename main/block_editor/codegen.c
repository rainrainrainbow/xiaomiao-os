/*
 * codegen.c - 代码生成器
 *
 * 将积木程序树翻译为完整的 MicroPython 源码
 * 输出格式:
 *   from xiaomiao import *
 *   import time
 *
 *   def on_start():
 *       ...积木代码...
 *
 *   if __name__ == '__main__':
 *       on_start()
 */
#include <string.h>
#include "lvgl.h"
#include "block_editor/codegen.h"
#include "block_editor/block_tree.h"
#include "block_editor/block_def.h"

int codegen_generate(program_tree_t *tree, char *buf, int buf_size)
{
    return program_tree_generate_code(tree, buf, buf_size);
}

void codegen_save_to_file(program_tree_t *tree, const char *filepath)
{
    char buf[4096];
    int len = codegen_generate(tree, buf, sizeof(buf));

    FILE *f = fopen(filepath, "w");
    if (f) {
        fwrite(buf, 1, len, f);
        fclose(f);
    }
}

int codegen_compile_check(const char *code, char *error_buf, int error_size)
{
    /* 简易语法检查（不依赖 MicroPython 引擎） */
    int parens = 0, brackets = 0, braces = 0;
    const char *p = code;
    while (*p) {
        if (*p == '(') parens++;
        if (*p == ')') parens--;
        if (*p == '[') brackets++;
        if (*p == ']') brackets--;
        if (*p == '{') braces++;
        if (*p == '}') braces--;
        p++;
    }
    if (parens || brackets || braces) {
        if (error_buf && error_size > 0) {
            snprintf(error_buf, error_size, "括号不匹配: (%d) [%d] {%d}",
                     parens, brackets, braces);
        }
        return -1;
    }
    return 0;
}
