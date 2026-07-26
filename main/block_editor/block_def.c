/*
 * block_def.c - 积木定义（6 分类 × 15 块）
 *
 * 每块积木包含: 类型/分类/显示名/代码模板/参数列表/是否可嵌套
 */
#include <string.h>
#include "block_editor/block_def.h"

/* 按键选项 */
static const char *KEY_CHOICES[] = {"A", "B", "UP", "DOWN", "LEFT", "RIGHT"};
/* 比较选项 */
static const char *CMP_CHOICES[] = {"<", ">", "==", "!="};

/* ============ 积木定义表 ============ */
static const block_def_t BLOCK_DEFS[] = {
    /* ---- 事件 (100+) ---- */
    {
        .type = BLOCK_EVENT_START, .cat = BLOCK_CAT_EVENT,
        .name = "当开机时", .code_tpl = "def on_start():\n",
        .param_count = 0, .has_children = true, .end_code = ""
    },
    {
        .type = BLOCK_EVENT_KEY_A, .cat = BLOCK_CAT_EVENT,
        .name = "当按键A按下", .code_tpl = "def on_key_a():\n",
        .param_count = 0, .has_children = true, .end_code = ""
    },
    {
        .type = BLOCK_EVENT_SHAKE, .cat = BLOCK_CAT_EVENT,
        .name = "当摇一摇时", .code_tpl = "def on_shake():\n",
        .param_count = 0, .has_children = true, .end_code = ""
    },

    /* ---- 控制 (200+) ---- */
    {
        .type = BLOCK_CTRL_IF, .cat = BLOCK_CAT_CONTROL,
        .name = "如果...则", .code_tpl = "if %s:\n",
        .param_count = 1,
        .params = {{
            .name = "条件", .type = PARAM_CHOICE, .default_val = "key_a_pressed()",
            .choices = (const char *[]){"key_a_pressed()","key_b_pressed()","battery()>3.5"},
            .choices_count = 3
        }},
        .has_children = true, .end_code = ""
    },
    {
        .type = BLOCK_CTRL_REPEAT, .cat = BLOCK_CAT_CONTROL,
        .name = "重复N次", .code_tpl = "for _ in range(%s):\n",
        .param_count = 1,
        .params = {{
            .name = "次数", .type = PARAM_NUMBER, .default_val = "10"
        }},
        .has_children = true, .end_code = ""
    },
    {
        .type = BLOCK_CTRL_FOREVER, .cat = BLOCK_CAT_CONTROL,
        .name = "永远循环", .code_tpl = "while True:\n",
        .param_count = 0, .has_children = true, .end_code = ""
    },
    {
        .type = BLOCK_CTRL_WAIT, .cat = BLOCK_CAT_CONTROL,
        .name = "等待N秒", .code_tpl = "time.sleep(%s)\n",
        .param_count = 1,
        .params = {{
            .name = "秒数", .type = PARAM_NUMBER, .default_val = "1"
        }},
        .has_children = false, .end_code = ""
    },

    /* ---- 显示 (300+) ---- */
    {
        .type = BLOCK_DISP_TEXT, .cat = BLOCK_CAT_DISPLAY,
        .name = "显示文字", .code_tpl = "screen.text(\"%s\")\n",
        .param_count = 1,
        .params = {{
            .name = "文字", .type = PARAM_TEXT, .default_val = "Hello"
        }},
        .has_children = false, .end_code = ""
    },
    {
        .type = BLOCK_DISP_RECT, .cat = BLOCK_CAT_DISPLAY,
        .name = "画矩形", .code_tpl = "screen.rect(%s,%s,%s,%s)\n",
        .param_count = 4,
        .params = {
            {.name="x",.type=PARAM_NUMBER,.default_val="0"},
            {.name="y",.type=PARAM_NUMBER,.default_val="0"},
            {.name="w",.type=PARAM_NUMBER,.default_val="50"},
            {.name="h",.type=PARAM_NUMBER,.default_val="30"},
        },
        .has_children = false, .end_code = ""
    },
    {
        .type = BLOCK_DISP_CLEAR, .cat = BLOCK_CAT_DISPLAY,
        .name = "清屏", .code_tpl = "screen.clear()\n",
        .param_count = 0, .has_children = false, .end_code = ""
    },

    /* ---- 按键 (400+) ---- */
    {
        .type = BLOCK_KEY_PRESSED, .cat = BLOCK_CAT_KEY,
        .name = "按键被按下?", .code_tpl = "key_%s_pressed()",
        .param_count = 1,
        .params = {{
            .name = "按键", .type = PARAM_CHOICE, .default_val = "a",
            .choices = KEY_CHOICES, .choices_count = 6
        }},
        .has_children = false, .end_code = ""
    },
    {
        .type = BLOCK_KEY_WAIT, .cat = BLOCK_CAT_KEY,
        .name = "等待按键", .code_tpl = "key.wait_press()\n",
        .param_count = 0, .has_children = false, .end_code = ""
    },

    /* ---- 传感器 (500+) ---- */
    {
        .type = BLOCK_SENS_ACC_X, .cat = BLOCK_CAT_SENSOR,
        .name = "加速度X", .code_tpl = "sensor.acc_x()",
        .param_count = 0, .has_children = false, .end_code = ""
    },
    {
        .type = BLOCK_SENS_BATTERY, .cat = BLOCK_CAT_SENSOR,
        .name = "电池电压", .code_tpl = "sensor.battery()",
        .param_count = 0, .has_children = false, .end_code = ""
    },
    {
        .type = BLOCK_SENS_GYRO_Z, .cat = BLOCK_CAT_SENSOR,
        .name = "陀螺仪Z", .code_tpl = "sensor.gyro_z()",
        .param_count = 0, .has_children = false, .end_code = ""
    },

    /* ---- 声音 (600+) ---- */
    {
        .type = BLOCK_SND_TONE, .cat = BLOCK_CAT_SOUND,
        .name = "播放音符", .code_tpl = "music.tone(%s,%s)\n",
        .param_count = 2,
        .params = {
            {.name="频率",.type=PARAM_NUMBER,.default_val="440"},
            {.name="时长",.type=PARAM_NUMBER,.default_val="0.5"},
        },
        .has_children = false, .end_code = ""
    },
    {
        .type = BLOCK_SND_STOP, .cat = BLOCK_CAT_SOUND,
        .name = "停止声音", .code_tpl = "music.stop()\n",
        .param_count = 0, .has_children = false, .end_code = ""
    },
};

static const int BLOCK_DEFS_COUNT = sizeof(BLOCK_DEFS) / sizeof(BLOCK_DEFS[0]);

/* 分类名 + 图标 */
static const char *CAT_NAMES[] = {"事件", "控制", "显示", "按键", "传感器", "声音"};
static const char *CAT_ICONS[] = {"E", "C", "D", "K", "S", "M"};

/* ============ API 实现 ============ */
const block_def_t *block_get_def(block_type_t type)
{
    for (int i = 0; i < BLOCK_DEFS_COUNT; i++) {
        if (BLOCK_DEFS[i].type == type) return &BLOCK_DEFS[i];
    }
    return NULL;
}

const block_def_t *block_get_by_index(block_cat_t cat, int idx)
{
    int count = 0;
    for (int i = 0; i < BLOCK_DEFS_COUNT; i++) {
        if (BLOCK_DEFS[i].cat == cat) {
            if (count == idx) return &BLOCK_DEFS[i];
            count++;
        }
    }
    return NULL;
}

int block_count_in_cat(block_cat_t cat)
{
    int count = 0;
    for (int i = 0; i < BLOCK_DEFS_COUNT; i++) {
        if (BLOCK_DEFS[i].cat == cat) count++;
    }
    return count;
}

const char *block_cat_name(block_cat_t cat)
{
    if (cat < 0 || cat >= BLOCK_CAT_COUNT) return "?";
    return CAT_NAMES[cat];
}

const char *block_cat_icon(block_cat_t cat)
{
    if (cat < 0 || cat >= BLOCK_CAT_COUNT) return "?";
    return CAT_ICONS[cat];
}

/* ============ 节点操作 ============ */
block_node_t *block_node_create(block_type_t type)
{
    block_node_t *n = lv_malloc_zero(sizeof(block_node_t));
    if (!n) return NULL;
    n->type = type;
    const block_def_t *def = block_get_def(type);
    if (def) {
        for (int i = 0; i < def->param_count && i < 4; i++) {
            strcpy(n->params[i], def->params[i].default_val);
        }
    }
    return n;
}

void block_node_free(block_node_t *node)
{
    if (!node) return;
    /* 递归释放子节点 */
    block_node_t *c = node->children;
    while (c) {
        block_node_t *next = c->next;
        block_node_free(c);
        c = next;
    }
    lv_free(node);
}

void block_node_add_child(block_node_t *parent, block_node_t *child)
{
    if (!parent || !child) return;
    child->parent = parent;
    child->indent = parent->indent + 1;
    /* 插入到子链表末尾 */
    if (!parent->children) {
        parent->children = child;
    } else {
        block_node_t *c = parent->children;
        while (c->next) c = c->next;
        c->next = child;
    }
}

void block_node_remove(block_node_t *node)
{
    if (!node || !node->parent) return;
    block_node_t *prev = NULL;
    block_node_t *cur = node->parent->children;
    while (cur && cur != node) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == node) {
        if (prev) prev->next = node->next;
        else node->parent->children = node->next;
    }
    block_node_free(node);
}

/* ============ 代码生成 ============ */
static void append_indent(char *buf, int *pos, int indent, int buf_size)
{
    for (int i = 0; i < indent; i++) {
        if (*pos + 4 < buf_size) {
            buf[*pos++] = ' '; buf[*pos++] = ' ';
            buf[*pos++] = ' '; buf[*pos++] = ' ';
        }
    }
}

static void append_str(char *buf, int *pos, const char *s, int buf_size)
{
    if (!s) return;
    while (*s && *pos < buf_size - 1) buf[(*pos)++] = *s++;
}

int block_tree_to_code(block_node_t *root, char *buf, int buf_size)
{
    int pos = 0;
    memset(buf, 0, buf_size);

    /* 文件头 */
    append_str(buf, &pos, "# Auto-generated by XiaoMiao Block Editor\n", buf_size);
    append_str(buf, &pos, "from xiaomiao import *\n\n", buf_size);

    /* 遍历所有顶层节点 */
    block_node_t *cur = root;
    while (cur && pos < buf_size - 1) {
        const block_def_t *def = block_get_def(cur->type);
        if (!def) { cur = cur->next; continue; }

        append_indent(buf, &pos, cur->indent, buf_size);

        if (def->param_count == 0) {
            append_str(buf, &pos, def->code_tpl, buf_size);
        } else if (def->param_count == 1) {
            /* 简单替换 %s */
            const char *tpl = def->code_tpl;
            while (*tpl && pos < buf_size - 1) {
                if (*tpl == '%' && *(tpl+1) == 's') {
                    append_str(buf, &pos, cur->params[0], buf_size);
                    tpl += 2;
                } else {
                    buf[pos++] = *tpl++;
                }
            }
        } else if (def->param_count == 2) {
            /* 如 music.tone(freq,dur) */
            const char *tpl = def->code_tpl;
            int pidx = 0;
            while (*tpl && pos < buf_size - 1) {
                if (*tpl == '%' && *(tpl+1) == 's') {
                    append_str(buf, &pos, cur->params[pidx], buf_size);
                    pidx++;
                    tpl += 2;
                } else {
                    buf[pos++] = *tpl++;
                }
            }
        } else if (def->param_count == 4) {
            /* screen.rect(x,y,w,h) */
            const char *tpl = def->code_tpl;
            int pidx = 0;
            while (*tpl && pos < buf_size - 1) {
                if (*tpl == '%' && *(tpl+1) == 's') {
                    append_str(buf, &pos, cur->params[pidx], buf_size);
                    pidx++;
                    tpl += 2;
                } else {
                    buf[pos++] = *tpl++;
                }
            }
        }

        /* 递归处理子节点 */
        if (def->has_children && cur->children) {
            block_node_t *child = cur->children;
            while (child && pos < buf_size - 1) {
                const block_def_t *cdef = block_get_def(child->type);
                if (cdef) {
                    append_indent(buf, &pos, child->indent, buf_size);
                    /* 简化：直接输出子节点代码 */
                    if (cdef->param_count == 0) {
                        append_str(buf, &pos, cdef->code_tpl, buf_size);
                    } else {
                        const char *tpl = cdef->code_tpl;
                        int pidx = 0;
                        while (*tpl && pos < buf_size - 1) {
                            if (*tpl == '%' && *(tpl+1) == 's') {
                                append_str(buf, &pos, child->params[pidx], buf_size);
                                pidx++;
                                tpl += 2;
                            } else {
                                buf[pos++] = *tpl++;
                            }
                        }
                    }
                    /* 孙节点 */
                    block_node_t *gc = child->children;
                    while (gc && pos < buf_size - 1) {
                        const block_def_t *gcdef = block_get_def(gc->type);
                        if (gcdef) {
                            append_indent(buf, &pos, gc->indent, buf_size);
                            if (gcdef->param_count == 0) {
                                append_str(buf, &pos, gcdef->code_tpl, buf_size);
                            } else {
                                const char *tpl = gcdef->code_tpl;
                                int pidx2 = 0;
                                while (*tpl && pos < buf_size - 1) {
                                    if (*tpl == '%' && *(tpl+1) == 's') {
                                        append_str(buf, &pos, gc->params[pidx2], buf_size);
                                        pidx2++;
                                        tpl += 2;
                                    } else {
                                        buf[pos++] = *tpl++;
                                    }
                                }
                            }
                        }
                        gc = gc->next;
                    }
                }
                child = child->next;
            }
            /* end_code */
            if (def->end_code && *def->end_code) {
                append_indent(buf, &pos, cur->indent, buf_size);
                append_str(buf, &pos, def->end_code, buf_size);
            }
        }

        cur = cur->next;
    }

    /* 调用入口函数 */
    append_str(buf, &pos, "\n# Auto-run entry\n", buf_size);
    append_str(buf, &pos, "if __name__ == '__main__':\n", buf_size);
    append_str(buf, &pos, "    on_start()\n", buf_size);

    return pos;
}
