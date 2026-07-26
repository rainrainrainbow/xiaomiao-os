#pragma once
#include "lvgl.h"

/* 页面 ID */
typedef enum {
    PAGE_DESKTOP = 0,
    PAGE_APP_LIST,
    PAGE_SETTINGS,
    PAGE_BLOCK_EDITOR,
    PAGE_APP_RUNNING,
    PAGE_COUNT
} page_id_t;

/* 页面切换回调 */
typedef void (*page_cb_t)(void *user_data);

void page_manager_init(void);
void page_manager_show(page_id_t page);
page_id_t page_manager_current(void);
void page_manager_set_data(void *data);
void *page_manager_get_data(void);
void page_manager_register_back_handler(page_cb_t cb);
void page_manager_back(void);
