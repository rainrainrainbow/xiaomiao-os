/**
 * @file ui_main.c
 * @brief 主UI界面实现 - 基于LVGL的桌面系统
 */

#include "ui_main.h"
#include "app_manager.h"
#include "lvgl_port.h"
#include "config_manager.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "ui_main";

// 屏幕尺寸
#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 128

// 应用网格配置
#define GRID_COLS     3
#define GRID_ROWS     2
#define APPS_PER_PAGE (GRID_COLS * GRID_ROWS)

// 最大运行应用数
#define MAX_RUNNING_APPS 8

// UI状态
typedef enum {
    UI_SCREEN_HOME,
    UI_SCREEN_APP_LIST,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_TASK_MANAGER,
    UI_SCREEN_ABOUT,
    UI_SCREEN_THEME
} ui_screen_t;

// 运行中的应用信息
typedef struct {
    char id[64];
    char name[64];
    bool locked;
} running_app_t;

// 全局UI状态
static ui_screen_t s_current_screen = UI_SCREEN_HOME;
static uint32_t s_current_page = 0;
static uint32_t s_selected_index = 0;
static uint32_t s_total_pages = 1;

// 运行中的应用列表
static running_app_t s_running_apps[MAX_RUNNING_APPS];
static uint32_t s_running_count = 0;
static uint32_t s_task_selected = 0;

// 设置菜单项
typedef enum {
    SETTING_THEME,
    SETTING_WIFI,
    SETTING_BRIGHTNESS,
    SETTING_VOLUME,
    SETTING_ABOUT,
    SETTING_COUNT
} setting_item_t;

// LVGL对象
static lv_obj_t *s_status_bar = NULL;
static lv_obj_t *s_time_label = NULL;
static lv_obj_t *s_battery_label = NULL;
static lv_obj_t *s_content_area = NULL;
static lv_obj_t *s_page_dots[4] = {NULL};
static lv_obj_t *s_app_icons[APPS_PER_PAGE] = {NULL};
static lv_obj_t *s_selected_highlight = NULL;

// 主题定义
typedef struct {
    const char *name;
    uint32_t primary;
    uint32_t bg;
    uint32_t text;
} theme_t;

static const theme_t s_themes[] = {
    {"默认",   0xF6D34A, 0x1B1713, 0xFFF3B0},  // 金黄色
    {"海洋",   0x4FC3F7, 0x0D1B2A, 0xE0F7FA},  // 蓝色
    {"森林",   0x66BB6A, 0x1B2E1B, 0xE8F5E9},  // 绿色
    {"星空",   0xBA68C8, 0x1A1A2E, 0xF3E5F5},  // 紫色
    {"火焰",   0xFF7043, 0x2E1B1B, 0xFFE0B2},  // 橙色
    {"冰晶",   0x4DD0E1, 0x1B2E2E, 0xE0F7FA},  // 青色
};
#define THEME_COUNT (sizeof(s_themes) / sizeof(s_themes[0]))
static uint32_t s_current_theme = 0;

// 主题颜色
static lv_color_t s_theme_primary = {0};
static lv_color_t s_theme_bg = {0};
static lv_color_t s_theme_text = {0};

// 应用主题并持久化
static void apply_theme(uint32_t theme_idx)
{
    if (theme_idx >= THEME_COUNT) theme_idx = 0;
    s_current_theme = theme_idx;
    s_theme_primary = lv_color_hex(s_themes[theme_idx].primary);
    s_theme_bg = lv_color_hex(s_themes[theme_idx].bg);
    s_theme_text = lv_color_hex(s_themes[theme_idx].text);
    
    // 更新屏幕背景
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, s_theme_bg, 0);
    
    // 持久化主题到配置文件
    config_manager_set_int(CONFIG_KEY_THEME, (int)theme_idx);
    config_manager_save();
    
    ESP_LOGI(TAG, "Theme applied: %s", s_themes[theme_idx].name);
}

// 从配置加载主题，若无配置则使用默认
static void load_theme_from_config(void)
{
    int theme_idx = config_manager_get_int(CONFIG_KEY_THEME, 0);
    apply_theme((uint32_t)theme_idx);
}

// 创建状态栏
static void create_status_bar(lv_obj_t *parent)
{
    s_status_bar = lv_obj_create(parent);
    lv_obj_set_size(s_status_bar, SCREEN_WIDTH, 12);
    lv_obj_align(s_status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(s_status_bar, lv_color_hex(0x2a2520), 0);
    lv_obj_set_style_border_width(s_status_bar, 0, 0);
    lv_obj_set_style_radius(s_status_bar, 0, 0);
    lv_obj_set_style_pad_all(s_status_bar, 2, 0);
    lv_obj_clear_flag(s_status_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    // 时间标签
    s_time_label = lv_label_create(s_status_bar);
    lv_label_set_text(s_time_label, "12:00");
    lv_obj_set_style_text_color(s_time_label, s_theme_primary, 0);
    lv_obj_set_style_text_font(s_time_label, &lv_font_montserrat_10, 0);
    lv_obj_align(s_time_label, LV_ALIGN_LEFT_MID, 0, 0);
    
    // 电池标签
    s_battery_label = lv_label_create(s_status_bar);
    lv_label_set_text(s_battery_label, "85%");
    lv_obj_set_style_text_color(s_battery_label, s_theme_primary, 0);
    lv_obj_set_style_text_font(s_battery_label, &lv_font_montserrat_10, 0);
    lv_obj_align(s_battery_label, LV_ALIGN_RIGHT_MID, 0, 0);
}

// 创建页面指示器
static void create_page_dots(lv_obj_t *parent, uint32_t total_pages)
{
    lv_obj_t *dots_container = lv_obj_create(parent);
    lv_obj_set_size(dots_container, SCREEN_WIDTH, 8);
    lv_obj_align(dots_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(dots_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots_container, 0, 0);
    lv_obj_clear_flag(dots_container, LV_OBJ_FLAG_SCROLLABLE);
    
    uint32_t dot_width = 4;
    uint32_t dot_gap = 4;
    uint32_t total_width = total_pages * dot_width + (total_pages - 1) * dot_gap;
    int32_t start_x = -(int32_t)(total_width / 2);
    
    for (uint32_t i = 0; i < total_pages && i < 4; i++) {
        s_page_dots[i] = lv_obj_create(dots_container);
        lv_obj_set_size(s_page_dots[i], dot_width, dot_width);
        lv_obj_set_style_radius(s_page_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_page_dots[i], 0, 0);
        
        if (i == s_current_page) {
            lv_obj_set_style_bg_color(s_page_dots[i], s_theme_primary, 0);
        } else {
            lv_obj_set_style_bg_color(s_page_dots[i], lv_color_hex(0x8B7355), 0);
        }
        
        lv_obj_align(s_page_dots[i], LV_ALIGN_LEFT_MID, start_x + i * (dot_width + dot_gap), 0);
        lv_obj_clear_flag(s_page_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }
}

// 创建应用图标
static void create_app_icon(lv_obj_t *parent, const app_info_t *app, uint32_t index)
{
    uint32_t col = index % GRID_COLS;
    uint32_t row = index / GRID_COLS;
    
    lv_obj_t *icon_container = lv_obj_create(parent);
    lv_obj_set_size(icon_container, 48, 48);
    lv_obj_set_style_bg_opa(icon_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_container, 0, 0);
    lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // 图标背景
    lv_obj_t *icon_bg = lv_obj_create(icon_container);
    lv_obj_set_size(icon_bg, 24, 24);
    lv_obj_set_style_bg_color(icon_bg, s_theme_primary, 0);
    lv_obj_set_style_radius(icon_bg, 6, 0);
    lv_obj_set_style_border_width(icon_bg, 0, 0);
    lv_obj_align(icon_bg, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE);
    
    // 应用名称
    lv_obj_t *name_label = lv_label_create(icon_container);
    lv_label_set_text(name_label, app->name);
    lv_obj_set_style_text_color(name_label, s_theme_text, 0);
    lv_obj_set_style_text_font(name_label, &lv_font_montserrat_8, 0);
    lv_obj_align(name_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // 定位
    int32_t x = 8 + col * 52;
    int32_t y = 4 + row * 52;
    lv_obj_align(icon_container, LV_ALIGN_TOP_LEFT, x, y);
    
    s_app_icons[index] = icon_container;
}

// 创建主屏幕
static void create_home_screen(void)
{
    // 清空内容区域
    lv_obj_clean(s_content_area);
    
    // 获取应用列表
    app_list_t *list = app_manager_get_list();
    if (!list || list->count == 0) {
        lv_obj_t *no_apps = lv_label_create(s_content_area);
        lv_label_set_text(no_apps, "无应用");
        lv_obj_set_style_text_color(no_apps, s_theme_text, 0);
        lv_obj_center(no_apps);
        return;
    }
    
    // 计算总页数
    s_total_pages = (list->count + APPS_PER_PAGE - 1) / APPS_PER_PAGE;
    
    // 创建页面指示器
    create_page_dots(s_content_area, s_total_pages);
    
    // 创建应用图标网格
    uint32_t start_idx = s_current_page * APPS_PER_PAGE;
    uint32_t end_idx = start_idx + APPS_PER_PAGE;
    if (end_idx > list->count) {
        end_idx = list->count;
    }
    
    for (uint32_t i = start_idx; i < end_idx; i++) {
        create_app_icon(s_content_area, &list->apps[i], i - start_idx);
    }
}

// 更新时间显示
static void update_time_display(lv_timer_t *timer)
{
    // 获取真实时间
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char time_str[8];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
    lv_label_set_text(s_time_label, time_str);
}

// 创建设置界面
static void create_settings_screen(void)
{
    lv_obj_clean(s_content_area);
    
    const char *items[] = {
        "🎨 主题",
        "📶 WiFi",
        "🔆 亮度",
        "🔊 音量",
        "ℹ️ 关于"
    };
    
    for (int i = 0; i < SETTING_COUNT; i++) {
        lv_obj_t *item = lv_obj_create(s_content_area);
        lv_obj_set_size(item, SCREEN_WIDTH - 8, 18);
        lv_obj_set_style_bg_opa(item, (i == s_selected_index) ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(item, s_theme_primary, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_set_style_radius(item, 4, 0);
        lv_obj_set_style_pad_left(item, 8, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, items[i]);
        lv_obj_set_style_text_color(label, s_theme_text, 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
        
        // 箭头
        lv_obj_t *arrow = lv_label_create(item);
        lv_label_set_text(arrow, ">");
        lv_obj_set_style_text_color(arrow, s_theme_primary, 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -8, 0);
        
        lv_obj_align(item, LV_ALIGN_TOP_MID, 0, 4 + i * 20);
    }
}

// 创建主题选择界面
static void create_theme_screen(void)
{
    lv_obj_clean(s_content_area);
    
    // 标题
    lv_obj_t *title = lv_label_create(s_content_area);
    lv_label_set_text(title, "选择主题");
    lv_obj_set_style_text_color(title, s_theme_text, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);
    
    // 主题网格 (2列3行)
    for (uint32_t i = 0; i < THEME_COUNT; i++) {
        uint32_t col = i % 2;
        uint32_t row = i / 2;
        
        lv_obj_t *item = lv_obj_create(s_content_area);
        lv_obj_set_size(item, 70, 28);
        lv_obj_set_style_bg_color(item, lv_color_hex(s_themes[i].primary), 0);
        lv_obj_set_style_bg_opa(item, (i == s_selected_index) ? LV_OPA_COVER : LV_OPA_50, 0);
        lv_obj_set_style_border_width(item, (i == s_current_theme) ? 2 : 0, 0);
        lv_obj_set_style_border_color(item, s_theme_text, 0);
        lv_obj_set_style_radius(item, 4, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, s_themes[i].name);
        lv_obj_set_style_text_color(label, lv_color_hex(s_themes[i].bg), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
        lv_obj_center(label);
        
        int32_t x = 8 + col * 76;
        int32_t y = 20 + row * 32;
        lv_obj_align(item, LV_ALIGN_TOP_LEFT, x, y);
    }
}

// 创建任务管理器界面
static void create_task_manager_screen(void)
{
    lv_obj_clean(s_content_area);
    
    // 标题
    lv_obj_t *title = lv_label_create(s_content_area);
    lv_label_set_text(title, "最近任务");
    lv_obj_set_style_text_color(title, s_theme_text, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);
    
    if (s_running_count == 0) {
        lv_obj_t *empty = lv_label_create(s_content_area);
        lv_label_set_text(empty, "暂无运行应用");
        lv_obj_set_style_text_color(empty, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(empty, &lv_font_montserrat_10, 0);
        lv_obj_align(empty, LV_ALIGN_CENTER, 0, 0);
        return;
    }
    
    // 任务列表
    for (uint32_t i = 0; i < s_running_count && i < 4; i++) {
        lv_obj_t *item = lv_obj_create(s_content_area);
        lv_obj_set_size(item, SCREEN_WIDTH - 8, 20);
        lv_obj_set_style_bg_opa(item, (i == s_task_selected) ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(item, s_theme_primary, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_set_style_border_side(item, LV_BORDER_SIDE_LEFT);
        lv_obj_set_style_border_color(item, s_running_apps[i].locked ? s_theme_primary : lv_color_hex(0x000000), 0);
        lv_obj_set_style_border_width(item, s_running_apps[i].locked ? 3 : 0, 0);
        lv_obj_set_style_radius(item, 4, 0);
        lv_obj_set_style_pad_left(item, 8, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // 应用名称
        lv_obj_t *name = lv_label_create(item);
        char name_text[70];
        snprintf(name_text, sizeof(name_text), "%s%s", 
                 s_running_apps[i].name,
                 s_running_apps[i].locked ? " 🔒" : "");
        lv_label_set_text(name, name_text);
        lv_obj_set_style_text_color(name, s_theme_text, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_10, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);
        
        // 操作按钮
        lv_obj_t *actions = lv_label_create(item);
        lv_label_set_text(actions, "A:进入 ←:锁定 →:销毁");
        lv_obj_set_style_text_color(actions, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(actions, &lv_font_montserrat_8, 0);
        lv_obj_align(actions, LV_ALIGN_RIGHT_MID, -4, 0);
        
        lv_obj_align(item, LV_ALIGN_TOP_MID, 0, 20 + i * 22);
    }
    
    // 底部提示
    lv_obj_t *hint = lv_label_create(s_content_area);
    lv_label_set_text(hint, "B:返回");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_8, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
}

// 创建关于界面
static void create_about_screen(void)
{
    lv_obj_clean(s_content_area);
    
    // Logo
    lv_obj_t *logo = lv_label_create(s_content_area);
    lv_label_set_text(logo, "🐱");
    lv_obj_set_style_text_font(logo, &lv_font_montserrat_28, 0);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 10);
    
    // 名称
    lv_obj_t *name = lv_label_create(s_content_area);
    lv_label_set_text(name, "XiaoMiao OS");
    lv_obj_set_style_text_color(name, s_theme_primary, 0);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 42);
    
    // 版本
    lv_obj_t *version = lv_label_create(s_content_area);
    lv_label_set_text(version, "v1.0.0");
    lv_obj_set_style_text_color(version, s_theme_text, 0);
    lv_obj_set_style_text_font(version, &lv_font_montserrat_10, 0);
    lv_obj_align(version, LV_ALIGN_TOP_MID, 0, 60);
    
    // 信息
    lv_obj_t *info = lv_label_create(s_content_area);
    lv_label_set_text(info, "ESP32-WROVER-B\n160x128 TFT\nMicroPython");
    lv_obj_set_style_text_color(info, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_8, 0);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 78);
    
    // 版权
    lv_obj_t *copyright = lv_label_create(s_content_area);
    lv_label_set_text(copyright, "© 2024 XiaoMiao Team");
    lv_obj_set_style_text_color(copyright, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(copyright, &lv_font_montserrat_8, 0);
    lv_obj_align(copyright, LV_ALIGN_BOTTOM_MID, 0, -8);
}

// 创建应用列表界面
static void create_app_list_screen(void)
{
    lv_obj_clean(s_content_area);
    
    app_list_t *list = app_manager_get_list();
    if (!list || list->count == 0) {
        lv_obj_t *empty = lv_label_create(s_content_area);
        lv_label_set_text(empty, "无应用");
        lv_obj_set_style_text_color(empty, s_theme_text, 0);
        lv_obj_center(empty);
        return;
    }
    
    // 列表视图
    for (uint32_t i = 0; i < list->count && i < 5; i++) {
        uint32_t idx = s_current_page * 5 + i;
        if (idx >= list->count) break;
        
        lv_obj_t *item = lv_obj_create(s_content_area);
        lv_obj_set_size(item, SCREEN_WIDTH - 8, 18);
        lv_obj_set_style_bg_opa(item, (i == s_selected_index) ? LV_OPA_30 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(item, s_theme_primary, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_set_style_radius(item, 4, 0);
        lv_obj_set_style_pad_left(item, 8, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        
        // 图标
        lv_obj_t *icon = lv_label_create(item);
        lv_label_set_text(icon, "📱");
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);
        
        // 名称
        lv_obj_t *name = lv_label_create(item);
        lv_label_set_text(name, list->apps[idx].name);
        lv_obj_set_style_text_color(name, s_theme_text, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_10, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 20, 0);
        
        // 版本
        lv_obj_t *ver = lv_label_create(item);
        lv_label_set_text(ver, list->apps[idx].version);
        lv_obj_set_style_text_color(ver, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(ver, &lv_font_montserrat_8, 0);
        lv_obj_align(ver, LV_ALIGN_RIGHT_MID, -8, 0);
        
        lv_obj_align(item, LV_ALIGN_TOP_MID, 0, 4 + i * 20);
    }
}

// 添加运行中的应用
void ui_add_running_app(const char *id, const char *name)
{
    if (s_running_count >= MAX_RUNNING_APPS) return;
    
    // 检查是否已存在
    for (uint32_t i = 0; i < s_running_count; i++) {
        if (strcmp(s_running_apps[i].id, id) == 0) {
            return;
        }
    }
    
    strncpy(s_running_apps[s_running_count].id, id, sizeof(s_running_apps[0].id) - 1);
    strncpy(s_running_apps[s_running_count].name, name, sizeof(s_running_apps[0].name) - 1);
    s_running_apps[s_running_count].locked = false;
    s_running_count++;
    
    ESP_LOGI(TAG, "App added to running: %s", name);
}

// 移除运行中的应用
void ui_remove_running_app(const char *id)
{
    for (uint32_t i = 0; i < s_running_count; i++) {
        if (strcmp(s_running_apps[i].id, id) == 0) {
            // 移动后面的元素
            for (uint32_t j = i; j < s_running_count - 1; j++) {
                s_running_apps[j] = s_running_apps[j + 1];
            }
            s_running_count--;
            if (s_task_selected >= s_running_count && s_task_selected > 0) {
                s_task_selected--;
            }
            ESP_LOGI(TAG, "App removed from running: %s", id);
            return;
        }
    }
}

// 锁定/解锁应用
void ui_toggle_app_lock(const char *id)
{
    for (uint32_t i = 0; i < s_running_count; i++) {
        if (strcmp(s_running_apps[i].id, id) == 0) {
            s_running_apps[i].locked = !s_running_apps[i].locked;
            ESP_LOGI(TAG, "App %s lock: %s", id, s_running_apps[i].locked ? "locked" : "unlocked");
            return;
        }
    }
}

// 检查应用是否锁定
bool ui_is_app_locked(const char *id)
{
    for (uint32_t i = 0; i < s_running_count; i++) {
        if (strcmp(s_running_apps[i].id, id) == 0) {
            return s_running_apps[i].locked;
        }
    }
    return false;
}

// 按键导航处理
void ui_handle_key(lv_key_t key)
{
    switch (s_current_screen) {
        case UI_SCREEN_HOME: {
            app_list_t *list = app_manager_get_list();
            if (!list || list->count == 0) return;
            
            uint32_t page_count = (list->count + APPS_PER_PAGE - 1) / APPS_PER_PAGE;
            
            switch (key) {
                case LV_KEY_UP:
                    if (s_selected_index >= GRID_COLS) {
                        s_selected_index -= GRID_COLS;
                    }
                    break;
                case LV_KEY_DOWN:
                    if (s_selected_index + GRID_COLS < APPS_PER_PAGE) {
                        s_selected_index += GRID_COLS;
                        // 检查是否超出当前页应用数
                        uint32_t start = s_current_page * APPS_PER_PAGE;
                        if (start + s_selected_index >= list->count) {
                            s_selected_index -= GRID_COLS;
                        }
                    }
                    break;
                case LV_KEY_LEFT:
                    if (s_selected_index > 0) {
                        s_selected_index--;
                    } else if (s_current_page > 0) {
                        s_current_page--;
                        s_selected_index = APPS_PER_PAGE - 1;
                        create_home_screen();
                    }
                    break;
                case LV_KEY_RIGHT:
                    if (s_selected_index < APPS_PER_PAGE - 1) {
                        uint32_t start = s_current_page * APPS_PER_PAGE;
                        if (start + s_selected_index + 1 < list->count) {
                            s_selected_index++;
                        } else if (s_current_page < page_count - 1) {
                            s_current_page++;
                            s_selected_index = 0;
                            create_home_screen();
                        }
                    }
                    break;
                case LV_KEY_ENTER: {
                    // 启动应用
                    uint32_t app_idx = s_current_page * APPS_PER_PAGE + s_selected_index;
                    if (app_idx < list->count) {
                        app_manager_launch_app(list->apps[app_idx].id);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        
        case UI_SCREEN_SETTINGS:
            switch (key) {
                case LV_KEY_UP:
                    if (s_selected_index > 0) s_selected_index--;
                    create_settings_screen();
                    break;
                case LV_KEY_DOWN:
                    if (s_selected_index < SETTING_COUNT - 1) s_selected_index++;
                    create_settings_screen();
                    break;
                case LV_KEY_ENTER:
                    switch (s_selected_index) {
                        case SETTING_THEME:
                            s_current_screen = UI_SCREEN_THEME;
                            s_selected_index = s_current_theme;
                            create_theme_screen();
                            break;
                        case SETTING_ABOUT:
                            s_current_screen = UI_SCREEN_ABOUT;
                            create_about_screen();
                            break;
                        default:
                            // TODO: 其他设置项
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
            
        case UI_SCREEN_THEME:
            switch (key) {
                case LV_KEY_UP:
                    if (s_selected_index >= 2) s_selected_index -= 2;
                    create_theme_screen();
                    break;
                case LV_KEY_DOWN:
                    if (s_selected_index + 2 < THEME_COUNT) s_selected_index += 2;
                    create_theme_screen();
                    break;
                case LV_KEY_LEFT:
                    if (s_selected_index > 0 && s_selected_index % 2 != 0) s_selected_index--;
                    create_theme_screen();
                    break;
                case LV_KEY_RIGHT:
                    if (s_selected_index + 1 < THEME_COUNT && s_selected_index % 2 == 0) s_selected_index++;
                    create_theme_screen();
                    break;
                case LV_KEY_ENTER:
                    apply_theme(s_selected_index);
                    s_current_screen = UI_SCREEN_SETTINGS;
                    s_selected_index = SETTING_THEME;
                    create_settings_screen();
                    break;
                default:
                    break;
            }
            break;
            
        case UI_SCREEN_TASK_MANAGER:
            if (s_running_count == 0) break;
            
            switch (key) {
                case LV_KEY_UP:
                    if (s_task_selected > 0) s_task_selected--;
                    create_task_manager_screen();
                    break;
                case LV_KEY_DOWN:
                    if (s_task_selected < s_running_count - 1) s_task_selected++;
                    create_task_manager_screen();
                    break;
                case LV_KEY_ENTER:
                    // 进入应用
                    ESP_LOGI(TAG, "Enter app: %s", s_running_apps[s_task_selected].name);
                    // TODO: 切换到应用界面
                    break;
                case LV_KEY_LEFT:
                    // 锁定/解锁
                    ui_toggle_app_lock(s_running_apps[s_task_selected].id);
                    create_task_manager_screen();
                    break;
                case LV_KEY_RIGHT:
                    // 销毁（如果未锁定）
                    if (!s_running_apps[s_task_selected].locked) {
                        app_manager_stop_app(s_running_apps[s_task_selected].id);
                        ui_remove_running_app(s_running_apps[s_task_selected].id);
                        create_task_manager_screen();
                    }
                    break;
                default:
                    break;
            }
            break;
            
        case UI_SCREEN_APP_LIST: {
            app_list_t *list = app_manager_get_list();
            if (!list || list->count == 0) break;
            
            uint32_t visible = 5;
            uint32_t total_pages = (list->count + visible - 1) / visible;
            
            switch (key) {
                case LV_KEY_UP:
                    if (s_selected_index > 0) s_selected_index--;
                    create_app_list_screen();
                    break;
                case LV_KEY_DOWN:
                    if (s_selected_index < visible - 1) {
                        uint32_t start = s_current_page * visible;
                        if (start + s_selected_index + 1 < list->count) {
                            s_selected_index++;
                        }
                    }
                    create_app_list_screen();
                    break;
                case LV_KEY_LEFT:
                    if (s_current_page > 0) {
                        s_current_page--;
                        s_selected_index = 0;
                        create_app_list_screen();
                    }
                    break;
                case LV_KEY_RIGHT:
                    if (s_current_page < total_pages - 1) {
                        s_current_page++;
                        s_selected_index = 0;
                        create_app_list_screen();
                    }
                    break;
                case LV_KEY_ENTER: {
                    uint32_t app_idx = s_current_page * visible + s_selected_index;
                    if (app_idx < list->count) {
                        app_manager_launch_app(list->apps[app_idx].id);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        
        default:
            break;
    }
}

esp_err_t ui_main_init(void)
{
    ESP_LOGI(TAG, "Initializing UI system");
    
    // 从配置文件加载主题（若无配置则使用默认主题）
    load_theme_from_config();
    
    // 创建主屏幕 (LVGL v9 API)
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, s_theme_bg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // 创建状态栏
    create_status_bar(screen);
    
    // 创建内容区域
    s_content_area = lv_obj_create(screen);
    lv_obj_set_size(s_content_area, SCREEN_WIDTH, SCREEN_HEIGHT - 12 - 8);
    lv_obj_align(s_content_area, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_bg_opa(s_content_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_content_area, 0, 0);
    lv_obj_clear_flag(s_content_area, LV_OBJ_FLAG_SCROLLABLE);
    
    // 创建主屏幕
    create_home_screen();
    
    // 创建时间更新定时器
    lv_timer_create(update_time_display, 60000, NULL);
    
    ESP_LOGI(TAG, "UI system initialized");
    return ESP_OK;
}

void ui_show_home(void)
{
    s_current_screen = UI_SCREEN_HOME;
    s_current_page = 0;
    s_selected_index = 0;
    create_home_screen();
}

void ui_show_app_list(void)
{
    s_current_screen = UI_SCREEN_APP_LIST;
    s_current_page = 0;
    s_selected_index = 0;
    create_app_list_screen();
}

void ui_show_settings(void)
{
    s_current_screen = UI_SCREEN_SETTINGS;
    s_selected_index = 0;
    create_settings_screen();
}

void ui_show_task_manager(void)
{
    s_current_screen = UI_SCREEN_TASK_MANAGER;
    s_task_selected = 0;
    create_task_manager_screen();
}

void ui_go_back(void)
{
    switch (s_current_screen) {
        case UI_SCREEN_THEME:
            s_current_screen = UI_SCREEN_SETTINGS;
            s_selected_index = SETTING_THEME;
            create_settings_screen();
            break;
        case UI_SCREEN_ABOUT:
            s_current_screen = UI_SCREEN_SETTINGS;
            s_selected_index = SETTING_ABOUT;
            create_settings_screen();
            break;
        case UI_SCREEN_SETTINGS:
        case UI_SCREEN_TASK_MANAGER:
        case UI_SCREEN_APP_LIST:
            ui_show_home();
            break;
        default:
            break;
    }
}

void ui_update_app_list(void)
{
    if (s_current_screen == UI_SCREEN_HOME) {
        create_home_screen();
    }
}

void ui_set_theme(const char *theme_name)
{
    for (uint32_t i = 0; i < THEME_COUNT; i++) {
        if (strcmp(s_themes[i].name, theme_name) == 0) {
            apply_theme(i);
            // 刷新当前界面
            switch (s_current_screen) {
                case UI_SCREEN_HOME: create_home_screen(); break;
                case UI_SCREEN_SETTINGS: create_settings_screen(); break;
                case UI_SCREEN_THEME: create_theme_screen(); break;
                case UI_SCREEN_TASK_MANAGER: create_task_manager_screen(); break;
                case UI_SCREEN_ABOUT: create_about_screen(); break;
                case UI_SCREEN_APP_LIST: create_app_list_screen(); break;
            }
            return;
        }
    }
    ESP_LOGW(TAG, "Theme not found: %s", theme_name);
}