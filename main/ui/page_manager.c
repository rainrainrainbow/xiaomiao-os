/*
 * page_manager.c - 页面管理
 *
 * 简单的页面栈，支持 push/pop/back
 * 每页全屏覆盖，切换时删除旧页面对象
 */
#include "lvgl.h"
#include "ui/page_manager.h"
#include "ui/theme.h"

#define MAX_PAGE_STACK 8

static page_id_t s_stack[MAX_PAGE_STACK];
static int       s_stack_top = 0;
static page_cb_t s_back_handler = NULL;
static void     *s_page_data = NULL;

/* 当前活动页面对象（每个页面应创建为 screen 的子对象） */
static lv_obj_t *s_pages[PAGE_COUNT] = {NULL};

void page_manager_init(void)
{
    s_stack_top = 0;
    s_stack[0] = PAGE_DESKTOP;
    s_page_data = NULL;
    for (int i = 0; i < PAGE_COUNT; i++) s_pages[i] = NULL;
}

void page_manager_show(page_id_t page)
{
    if (page < 0 || page >= PAGE_COUNT) return;

    /* 隐藏所有页面 */
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (s_pages[i]) lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
    }

    s_stack[++s_stack_top] = page;
    if (s_stack_top >= MAX_PAGE_STACK) s_stack_top = MAX_PAGE_STACK - 1;
}

page_id_t page_manager_current(void)
{
    return s_stack[s_stack_top];
}

void page_manager_set_data(void *data) { s_page_data = data; }
void *page_manager_get_data(void)      { return s_page_data; }

void page_manager_register_back_handler(page_cb_t cb) { s_back_handler = cb; }

void page_manager_back(void)
{
    if (s_stack_top > 0) {
        s_stack_top--;
        if (s_back_handler) s_back_handler(NULL);
    }
}

/* 注册页面根对象（供各页面模块调用） */
void page_register(page_id_t id, lv_obj_t *obj) { s_pages[id] = obj; }
lv_obj_t *page_get(page_id_t id) { return s_pages[id]; }
void page_show(page_id_t id)
{
    if (s_pages[id]) lv_obj_remove_flag(s_pages[id], LV_OBJ_FLAG_HIDDEN);
}
void page_hide(page_id_t id)
{
    if (s_pages[id]) lv_obj_add_flag(s_pages[id], LV_OBJ_FLAG_HIDDEN);
}
