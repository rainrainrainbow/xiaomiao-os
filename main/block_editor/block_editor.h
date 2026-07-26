/*
 * block_editor/block_editor.h — Visual Block Programming Editor
 *
 * Menu-style block editor (not drag-drop) for creating MicroPython
 * programs on-device. Blocks are selected from categories, inserted
 * into a program tree, and later compiled to .py code.
 */

#pragma once

#include "lvgl.h"
#include "ui/system.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Block types ──────────────────────────────────────────────────────── */
typedef enum {
    BLOCK_EVENT_START = 0,
    BLOCK_EVENT_KEYPRESS,
    BLOCK_EVENT_SHAKE,
    BLOCK_CTRL_REPEAT,
    BLOCK_CTRL_FOREVER,
    BLOCK_CTRL_IF,
    BLOCK_CTRL_WAIT,
    BLOCK_DISP_TEXT,
    BLOCK_DISP_RECT,
    BLOCK_DISP_CLEAR,
    BLOCK_KEY_A_PRESSED,
    BLOCK_KEY_DIR,
    BLOCK_SENSOR_ACC,
    BLOCK_SENSOR_BATTERY,
    BLOCK_SOUND_TONE,
    BLOCK_TYPE_COUNT
} block_type_t;

/* ── Block parameter ──────────────────────────────────────────────────── */
typedef struct {
    char name[16];
    char value_str[64];
    int  value_int;
    float value_float;
    int  param_type;  /* 0=string, 1=int, 2=float, 3=choice */
} block_param_t;

/* ── Block definition (read-only template) ───────────────────────────── */
typedef struct {
    block_type_t type;
    uint8_t      category;   /* 0=event,1=ctrl,2=disp,3=key,4=sensor,5=sound */
    const char  *display_name;
    const char  *code_template;  /* e.g. "screen.text(\"{text}\")" */
    int          param_count;
    const char  *param_names[4];
    int          param_types[4];  /* 0=string,1=int,2=float,3=choice */
    const char  *param_choices[4]; /* for type=3 */
    int          param_defaults_int[4];
    float        param_defaults_float[4];
    const char  *param_defaults_str[4];
} block_def_t;

/* ── Block instance (in a program) ───────────────────────────────────── */
typedef struct block_inst_t {
    block_type_t       type;
    char               name[32];
    uint8_t            indent;
    uint8_t            param_count;
    block_param_t      params[4];
    struct block_inst_t *children;
    struct block_inst_t *next;
} block_inst_t;

/* ── Program ──────────────────────────────────────────────────────────── */
typedef struct {
    char          name[32];
    uint16_t      version;
    block_inst_t *root;       /* linked list of top-level blocks */
    uint32_t      modified_ms;
} block_program_t;

/* ── Block editor state ──────────────────────────────────────────────── */
typedef enum {
    EDIT_MODE_BROWSE = 0,    /* browsing blocks */
    EDIT_MODE_EDIT_PARAM,    /* editing a parameter */
    EDIT_MODE_NESTING,       /* adjusting indent/nesting */
} edit_mode_t;

typedef struct {
    uint8_t      category;       /* current category (0-5) */
    int          list_cursor;    /* cursor in block list */
    int          tree_cursor;    /* cursor in program tree */
    edit_mode_t  mode;
    uint8_t      indent_level;
    block_program_t program;
} block_editor_state_t;

/* ── Public API ───────────────────────────────────────────────────────── */

/* Editor lifecycle */
void editor_init(void);
void editor_build(lv_obj_t *parent);

/* Block definitions */
const block_def_t *block_get_def(block_type_t type);
const block_def_t *block_get_def_at(int category, int index);
int block_count_in_category(int category);

/* Program manipulation */
block_inst_t *block_create(block_type_t type);
void block_free(block_inst_t *block);
void program_clear(block_program_t *prog);
void program_add_root(block_program_t *prog, block_inst_t *block);
void program_insert_child(block_inst_t *parent, block_inst_t *child);
void program_remove_at(block_program_t *prog, int index);

/* Code generation */
int codegen_generate(const block_program_t *prog, char *output, size_t out_size);

/* JSON save/load */
int program_save_to_json(const block_program_t *prog, char *json, size_t json_size);
int program_load_from_json(block_program_t *prog, const char *json);

/* Input handling */
void editor_handle_key(hal_key_t key);

/* Run current program */
void editor_run_program(void);

/* Editor state */
block_editor_state_t *editor_get_state(void);

#ifdef __cplusplus
}
#endif
