#pragma once
#include "lvgl.h"

/* 积木分类 */
typedef enum {
    BLOCK_CAT_EVENT = 0,   /* ⚡ 事件 */
    BLOCK_CAT_CONTROL,      /* 🔀 控制 */
    BLOCK_CAT_DISPLAY,      /* 🖥️ 显示 */
    BLOCK_CAT_KEY,          /* ⌨️ 按键 */
    BLOCK_CAT_SENSOR,       /* 📡 传感器 */
    BLOCK_CAT_SOUND,        /* 🔊 声音 */
    BLOCK_CAT_COUNT
} block_cat_t;

/* 积木类型 */
typedef enum {
    /* 事件 */
    BLOCK_EVENT_START = 100,
    BLOCK_EVENT_KEY_A,
    BLOCK_EVENT_KEY_B,
    BLOCK_EVENT_SHAKE,
    /* 控制 */
    BLOCK_CTRL_IF = 200,
    BLOCK_CTRL_IF_ELSE,
    BLOCK_CTRL_REPEAT,
    BLOCK_CTRL_FOREVER,
    BLOCK_CTRL_WAIT,
    /* 显示 */
    BLOCK_DISP_TEXT = 300,
    BLOCK_DISP_RECT,
    BLOCK_DISP_CLEAR,
    /* 按键 */
    BLOCK_KEY_PRESSED = 400,
    BLOCK_KEY_WAIT,
    /* 传感器 */
    BLOCK_SENS_ACC_X = 500,
    BLOCK_SENS_BATTERY,
    BLOCK_SENS_GYRO_Z,
    /* 声音 */
    BLOCK_SND_TONE = 600,
    BLOCK_SND_STOP,
    BLOCK_COUNT
} block_type_t;

/* 积木参数类型 */
typedef enum {
    PARAM_NONE = 0,
    PARAM_TEXT,     /* 文本输入 */
    PARAM_NUMBER,   /* 数字输入 */
    PARAM_CHOICE,   /* 下拉选择 */
    PARAM_EXPR,     /* 表达式 (嵌套) */
} param_type_t;

/* 积木参数定义 */
typedef struct {
    const char   *name;       /* 参数名 */
    param_type_t  type;       /* 参数类型 */
    const char   *default_val; /* 默认值 */
    const char  **choices;    /* 选项列表 (PARAM_CHOICE) */
    int           choices_count;
} block_param_def_t;

/* 积木定义 */
typedef struct {
    block_type_t      type;
    block_cat_t       cat;
    const char       *name;       /* 显示名 */
    const char       *code_tpl;   /* 代码模板 (含 %s 占位符) */
    int               param_count;
    block_param_def_t params[4];  /* 最多4个参数 */
    bool              has_children;/* 是否可嵌套子积木 */
    const char       *end_code;   /* 结束代码 (如 endif/endfor) */
} block_def_t;

/* 程序树节点 */
typedef struct block_node {
    block_type_t       type;
    char               params[4][32];  /* 参数值 */
    int                indent;          /* 缩进级别 */
    struct block_node *children;       /* 子节点链表 */
    struct block_node *next;           /* 兄弟节点 */
    struct block_node *parent;         /* 父节点 */
} block_node_t;

/* 获取积木定义 */
const block_def_t *block_get_def(block_type_t type);
const block_def_t *block_get_by_index(block_cat_t cat, int idx);
int                block_count_in_cat(block_cat_t cat);

/* 分类名 */
const char *block_cat_name(block_cat_t cat);
const char *block_cat_icon(block_cat_t cat);

/* 创建/销毁节点 */
block_node_t *block_node_create(block_type_t type);
void          block_node_free(block_node_t *node);
void          block_node_add_child(block_node_t *parent, block_node_t *child);
void          block_node_remove(block_node_t *node); /* 从树中移除并释放 */

/* 生成 MicroPython 代码 */
int  block_tree_to_code(block_node_t *root, char *buf, int buf_size);
