/*
 * input.c - 积木编辑器按键输入处理
 *
 * 交互流程:
 *   1. EDIT_MODE_CAT_SELECT: ←→ 选分类, ↓ 进入积木选择
 *   2. EDIT_MODE_BLOCK_SELECT: ↑↓ 选积木, A 插入, ← 返回分类
 *   3. EDIT_MODE_TREE_VIEW: ↑↓ 移动光标, A 编辑参数, B 删除
 *   4. EDIT_MODE_PARAM_EDIT: 数字增减/文本输入, A 确认
 */
#include <string.h>
#include "lvgl.h"
#include "block_editor/input.h"
#include "block_editor/block_tree.h"
#include "block_editor/block_def.h"
#include "block_editor/editor.h"

static program_tree_t *s_tree = NULL;
static edit_mode_t     s_mode = EDIT_MODE_CAT_SELECT;
static block_cat_t     s_cat = BLOCK_CAT_EVENT;
static int             s_block_idx = 0;
static int             s_param_idx = 0;
static char            s_param_buf[32];
static bool            s_param_editing = false;

void editor_input_init(program_tree_t *tree)
{
    s_tree = tree;
    s_mode = EDIT_MODE_CAT_SELECT;
    s_cat = BLOCK_CAT_EVENT;
    s_block_idx = 0;
}

edit_mode_t editor_get_mode(void) { return s_mode; }

void editor_start_param_edit(int param_idx)
{
    if (!s_tree || !s_tree->cursor) return;
    const block_def_t *def = block_get_def(s_tree->cursor->type);
    if (!def || param_idx >= def->param_count) return;

    s_param_idx = param_idx;
    strcpy(s_param_buf, s_tree->cursor->params[param_idx]);
    s_param_editing = true;
    s_mode = EDIT_MODE_PARAM_EDIT;
}

void editor_param_input_char(char c)
{
    int len = (int)strlen(s_param_buf);
    if (len < 31) {
        s_param_buf[len] = c;
        s_param_buf[len+1] = '\0';
    }
}

void editor_param_backspace(void)
{
    int len = (int)strlen(s_param_buf);
    if (len > 0) s_param_buf[len-1] = '\0';
}

void editor_param_confirm(void)
{
    if (!s_tree || !s_tree->cursor) return;
    strcpy(s_tree->cursor->params[s_param_idx], s_param_buf);
    s_param_editing = false;
    s_mode = EDIT_MODE_TREE_VIEW;
}

void editor_input_handle_key(int key)
{
    if (!s_tree) return;

    switch (s_mode) {

    case EDIT_MODE_CAT_SELECT:
        switch (key) {
            case 3: /* LEFT */  if (s_cat > 0) s_cat--; break;
            case 4: /* RIGHT */ if (s_cat < BLOCK_CAT_COUNT-1) s_cat++; break;
            case 2: /* DOWN */  s_mode = EDIT_MODE_BLOCK_SELECT; s_block_idx = 0; break;
            case 5: /* A */     s_mode = EDIT_MODE_BLOCK_SELECT; s_block_idx = 0; break;
        }
        break;

    case EDIT_MODE_BLOCK_SELECT:
        switch (key) {
            case 1: /* UP */   if (s_block_idx > 0) s_block_idx--; break;
            case 2: /* DOWN */ if (s_block_idx < block_count_in_cat(s_cat)-1) s_block_idx++; break;
            case 3: /* LEFT */ s_mode = EDIT_MODE_CAT_SELECT; break;
            case 5: /* A - 插入积木 */
                {
                    const block_def_t *def = block_get_by_index(s_cat, s_block_idx);
                    if (def) {
                        program_tree_insert(s_tree, def->type);
                        s_mode = EDIT_MODE_TREE_VIEW;
                    }
                }
                break;
            case 6: /* B */ s_mode = EDIT_MODE_CAT_SELECT; break;
        }
        break;

    case EDIT_MODE_TREE_VIEW:
        switch (key) {
            case 1: /* UP */   program_tree_move_up(s_tree); break;
            case 2: /* DOWN */ program_tree_move_down(s_tree); break;
            case 3: /* LEFT */ program_tree_indent_left(s_tree); break;
            case 4: /* RIGHT*/ program_tree_indent_right(s_tree); break;
            case 5: /* A - 编辑参数 */
                if (s_tree->cursor) {
                    const block_def_t *def = block_get_def(s_tree->cursor->type);
                    if (def && def->param_count > 0) {
                        editor_start_param_edit(0);
                    }
                }
                break;
            case 6: /* B - 删除 */
                program_tree_delete_current(s_tree);
                break;
        }
        break;

    case EDIT_MODE_PARAM_EDIT:
        switch (key) {
            case 1: /* UP - 数字+1 */
                {
                    int v = atoi(s_param_buf);
                    char buf[16]; snprintf(buf, sizeof(buf), "%d", v+1);
                    strcpy(s_param_buf, buf);
                }
                break;
            case 2: /* DOWN - 数字-1 */
                {
                    int v = atoi(s_param_buf);
                    char buf[16]; snprintf(buf, sizeof(buf), "%d", v>0?v-1:0);
                    strcpy(s_param_buf, buf);
                }
                break;
            case 3: /* LEFT - 退格 */
                editor_param_backspace();
                break;
            case 4: /* RIGHT - 输入 '.' 或 字母 */
                /* 简化：切换选项 */
                {
                    const block_def_t *def = s_tree->cursor ? block_get_def(s_tree->cursor->type) : NULL;
                    if (def && def->params[s_param_idx].type == PARAM_CHOICE) {
                        /* 循环切换选项 */
                        int ci = 0;
                        for (; ci < def->params[s_param_idx].choices_count; ci++) {
                            if (strcmp(s_param_buf, def->params[s_param_idx].choices[ci]) == 0) break;
                        }
                        ci = (ci + 1) % def->params[s_param_idx].choices_count;
                        strcpy(s_param_buf, def->params[s_param_idx].choices[ci]);
                    }
                }
                break;
            case 5: /* A - 确认 */
                editor_param_confirm();
                break;
            case 6: /* B - 取消 */
                s_param_editing = false;
                s_mode = EDIT_MODE_TREE_VIEW;
                break;
        }
        break;

    case EDIT_MODE_RUN:
        /* 运行模式：只有 B 返回编辑器 */
        if (key == 6) {
            s_mode = EDIT_MODE_TREE_VIEW;
        }
        break;
    }
}
