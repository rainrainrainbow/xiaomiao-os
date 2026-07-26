/*
 * block_tree.c - 程序树管理
 *
 * 操作: 插入/删除/移动/缩进/序列化
 */
#include <string.h>
#include "lvgl.h"
#include "block_editor/block_tree.h"
#include "block_editor/block_def.h"

program_tree_t *program_tree_create(void)
{
    program_tree_t *t = lv_malloc_zero(sizeof(program_tree_t));
    return t;
}

void program_tree_free(program_tree_t *tree)
{
    if (!tree) return;
    block_node_t *cur = tree->root;
    while (cur) {
        block_node_t *next = cur->next;
        block_node_free(cur);
        cur = next;
    }
    lv_free(tree);
}

/* 在光标后插入新节点 */
void program_tree_insert(program_tree_t *tree, block_type_t type)
{
    if (!tree) return;

    block_node_t *node = block_node_create(type);
    if (!node) return;

    if (!tree->root) {
        tree->root = node;
        tree->cursor = node;
    } else if (!tree->cursor) {
        /* 插入到末尾 */
        block_node_t *c = tree->root;
        while (c->next) c = c->next;
        c->next = node;
        node->parent = NULL;
        tree->cursor = node;
    } else {
        /* 插入到光标之后 */
        node->next = tree->cursor->next;
        tree->cursor->next = node;
        node->parent = tree->cursor->parent;
        node->indent = tree->cursor->indent;
        tree->cursor = node;
    }
    tree->node_count++;
}

void program_tree_delete_current(program_tree_t *tree)
{
    if (!tree || !tree->cursor) return;

    block_node_t *cur = tree->cursor;

    if (cur == tree->root) {
        tree->root = cur->next;
        tree->cursor = tree->root;
    } else {
        /* 找前驱 */
        block_node_t *prev = tree->root;
        while (prev && prev->next != cur) prev = prev->next;
        if (prev) {
            prev->next = cur->next;
            tree->cursor = prev;
        }
    }
    block_node_free(cur);
    tree->node_count--;
}

void program_tree_move_up(program_tree_t *tree)
{
    if (!tree || !tree->cursor || tree->cursor == tree->root) return;

    block_node_t *cur = tree->cursor;
    block_node_t *prev = tree->root;
    while (prev->next != cur) prev = prev->next;

    /* 交换 cur 和 prev */
    block_node_t *pp = tree->root;
    while (pp->next != prev) pp = pp->next;

    if (prev == tree->root) {
        tree->root = cur;
    } else {
        pp->next = cur;
    }
    prev->next = cur->next;
    cur->next = prev;

    tree->cursor = prev; /* 光标上移 */
}

void program_tree_move_down(program_tree_t *tree)
{
    if (!tree || !tree->cursor || !tree->cursor->next) return;
    tree->cursor = tree->cursor->next;
}

void program_tree_indent_right(program_tree_t *tree)
{
    if (!tree || !tree->cursor) return;
    if (tree->cursor->indent < 4) {
        tree->cursor->indent++;
    }
}

void program_tree_indent_left(program_tree_t *tree)
{
    if (!tree || !tree->cursor) return;
    if (tree->cursor->indent > 0) {
        tree->cursor->indent--;
    }
}

/* ========== JSON 序列化（简易版，无完整 JSON 库） ========== */
int program_tree_to_json(program_tree_t *tree, char *buf, int buf_size)
{
    int pos = 0;
    memset(buf, 0, buf_size);

    if (!tree || !tree->root) {
        snprintf(buf, buf_size, "{\"program\":{\"name\":\"\",\"blocks\":[]}}");
        return (int)strlen(buf);
    }

    #define JAPP(s) do { if (pos < buf_size-1) buf[pos++]=(s); } while(0)
    #define JSTR(s) do { const char*_p=(s); while(*_p&&pos<buf_size-1)buf[pos++]=*_p++; } while(0)

    JSTR("{\"program\":{\"name\":\"\",\"blocks\":[");
    /* 简化：只输出类型和参数 */
    block_node_t *c = tree->root;
    bool first = true;
    while (c && pos < buf_size - 1) {
        if (!first) JAPP(',');
        first = false;
        JAPP('{');
        JSTR("\"type\":");
        char num[8];
        snprintf(num, sizeof(num), "%d", c->type);
        JSTR(num);
        /* params */
        const block_def_t *def = block_get_def(c->type);
        if (def && def->param_count > 0) {
            JSTR(",\"params\":[");
            for (int i = 0; i < def->param_count && i < 4; i++) {
                if (i > 0) JAPP(',');
                JAPP('"'); JSTR(c->params[i]); JAPP('"');
            }
            JAPP(']');
        }
        JSTR(",\"indent\":");
        snprintf(num, sizeof(num), "%d", c->indent);
        JSTR(num);
        JAPP('}');
        c = c->next;
    }
    JSTR("]}}");

    #undef JAPP
    #undef JSTR
    return pos;
}

bool program_tree_from_json(program_tree_t *tree, const char *json)
{
    /* TODO: 完整 JSON 解析需要 cJSON 库 */
    /* 当前简化版本不支持从 JSON 恢复 */
    return false;
}

int program_tree_generate_code(program_tree_t *tree, char *buf, int buf_size)
{
    if (!tree || !tree->root) {
        snprintf(buf, buf_size, "from xiaomiao import *\n\n"
                 "def on_start():\n    pass\n\n"
                 "if __name__ == '__main__':\n    on_start()\n");
        return (int)strlen(buf);
    }
    return block_tree_to_code(tree->root, buf, buf_size);
}
