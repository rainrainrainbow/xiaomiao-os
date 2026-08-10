// ================ app_manager.c - App 注册 + 屏幕切换 ================

#include "app_manager.h"
#include "ui/theme.h"
#include "ui/toast.h"
#include "ui/canvas.h"
#include "hal/buzzer.h"
#include "core/settings.h"
#include "mpy/mp_runtime.h"
#include "mpy/mp_bindings.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "app_mgr";

// ================ 静态 App 注册表 ================
static const app_info_t kBuiltin[] = {
    {"sys.applist",        "应用",   "📦", "—",  true,  true},
    {"sys.settings",       "设置",   "⚙\u200d", "—",  true,  true},
    {"sys.editor",         "积木",   "🧩", "—",  true,  true},
    {"sys.store",          "商店",   "🛒", "—",  true,  true},
    {"com.demo.snake",     "贪吃蛇", "🐍", "1.0.2", false, true},
    {"com.demo.music",     "音乐",   "🎵", "0.9.0", false, true},
    {"com.demo.paint",     "画板",   "🎨", "1.1.0", false, true},
    {"com.demo.clock",     "时钟",   "⏰", "2.0.0", false, true},
    {"com.demo.weather",   "天气",   "🌤️", "1.0.0", false, true},
    {"com.demo.calc",      "计算器", "🔢", "1.0.1", false, true},
    {"com.demo.files",     "文件",   "📁", "1.2.0", false, true},
    {"com.demo.hello",     "Hello",  "👋", "0.1.0", false, true},
    {"com.demo.pixel",     "像素鸟", "🕹\u200d", "1.3.0", false, true},
    {"com.demo.timer",     "秒表",   "⏱\u200d", "1.0.0", false, true},
};

static const app_info_t kStore[] = {
    {"com.store.tetris", "俄罗斯方块", "🧱", "1.0.0", false, false},
    {"com.store.chess",  "中国象棋",   "♟\u200d", "1.0.0", false, false},
    {"com.store.reader", "电子书",     "📖", "1.0.0", false, false},
    {"com.store.radio",  "网络电台",   "📻", "1.0.0", false, false},
};

#define BUILTIN_COUNT  (sizeof(kBuiltin)/sizeof(kBuiltin[0]))
#define STORE_COUNT    (sizeof(kStore)/sizeof(kStore[0]))

// ================ 运行时状态 ================
typedef struct {
    // 5 屏
    lv_obj_t *screens[SCREEN_MAX];
    screen_id_t current;

    // 桌面网格
    lv_obj_t *grid;         // 当前页 container
    lv_obj_t *icons[6];     // 3x2 = 6
    lv_obj_t *dots[8];      // 最多 8 页
    lv_obj_t *dock;         // 装 dots

    // 应用列表 / 设置 / 商店 / 积木
    lv_obj_t *apps_list;
    lv_obj_t *settings_list;
    lv_obj_t *store_list;
    lv_obj_t *editor_block_panel;
    lv_obj_t *editor_prog_panel;
    lv_obj_t *editor_shortcut_label;

    // 运行中 App 显示
    lv_obj_t *app_glyph;
    lv_obj_t *app_name;
    lv_obj_t *app_status;

    // 状态栏
    lv_obj_t *statusbar;
    lv_obj_t *sb_time;
    lv_obj_t *sb_batt;
    lv_obj_t *sb_batt_fill;

    // 状态
    int  sel;
    int  page;
    int  list_sel;
    int  set_sel;
    int  store_sel;
    int  ed_pane;
    int  ed_block_sel;
    int  ed_prog_sel;
    bool move_mode;

    // 排序副本（编辑用，原列表只读）
    int  ordered_idx[14];  // 引用 kBuiltin 索引
} state_t;

static state_t *S = NULL;
static lv_obj_t *s_parent = NULL;

// ================ 辅助 ================
static lv_obj_t *mk_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_8, 0);
    return l;
}

// ================ 初始化所有 App 排序索引 ================
static void reset_order(void)
{
    for (int i = 0; i < (int)BUILTIN_COUNT; i++) {
        S->ordered_idx[i] = i;
    }
}

// ================ 桌面屏渲染 ================
static void desktop_render(void)
{
    if (!S->grid) return;
    int start = S->page * 6;
    int end = start + 6;
    if (end > (int)BUILTIN_COUNT) end = (int)BUILTIN_COUNT;

    for (int i = 0; i < 6; i++) {
        if (!S->icons[i]) continue;
        if (start + i < end) {
            const app_info_t *app = &kBuiltin[S->ordered_idx[start + i]];
            lv_obj_t *lbl = lv_obj_get_child(S->icons[i], 1);
            lv_obj_t *glyph = lv_obj_get_child(S->icons[i], 0);
            lv_label_set_text(glyph, app->glyph);
            lv_label_set_text(lbl, app->name);
            lv_obj_clear_flag(S->icons[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(S->icons[i], LV_OBJ_FLAG_HIDDEN);
        }
        bool sel = (i == S->sel);
        bool mov = sel && S->move_mode;
        style_apply_icon(S->icons[i], sel, mov);
    }

    // Dock dots
    int total = (BUILTIN_COUNT + 5) / 6;
    for (int i = 0; i < 8; i++) {
        if (i < total) {
            lv_obj_clear_flag(S->dots[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_opa(S->dots[i],
                i == S->page ? LV_OPA_COVER : LV_OPA_30, 0);
        } else {
            lv_obj_add_flag(S->dots[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ================ 应用列表渲染 ================
static void apps_list_render(void)
{
    if (!S->apps_list) return;
    lv_obj_clean(S->apps_list);
    int row = 0;
    for (int i = 0; i < (int)BUILTIN_COUNT; i++) {
        const app_info_t *app = &kBuiltin[S->ordered_idx[i]];
        if (app->is_system) continue;  // 桌面系统 App 单独显示

        lv_obj_t *item = lv_obj_create(S->apps_list);
        lv_obj_set_size(item, LV_HOR_RES - 6, 14);
        lv_obj_set_pos(item, 0, row * 14);
        style_apply_list_item(item, row == S->list_sel);

        lv_obj_t *l = lv_label_create(item);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_8, 0);
        char buf[24];
        snprintf(buf, sizeof(buf), "%s %s", app->glyph, app->name);
        lv_label_set_text(l, buf);
        lv_obj_set_pos(l, 3, 0);

        lv_obj_t *r = lv_label_create(item);
        lv_obj_set_style_text_font(r, &lv_font_montserrat_8, 0);
        lv_label_set_text(r, app->version);
        lv_obj_align(r, LV_ALIGN_RIGHT_MID, -3, 0);

        row++;
    }
}

// ================ 设置列表渲染 ================
static const char *wifi_str(void)
{
    return settings_wifi_on() ? "已连接" : "关闭";
}
static const char *vol_str(void)
{
    static char buf[8]; snprintf(buf, sizeof(buf), "%d%%", settings_volume());
    return buf;
}
static const char *batt_str(void)
{
    static char buf[8]; snprintf(buf, sizeof(buf), "%.2fV", settings_battery_v());
    return buf;
}

static void settings_render(void)
{
    if (!S->settings_list) return;
    lv_obj_clean(S->settings_list);
    static const struct { const char *k; const char *(*v)(void); } items[] = {
        {"Wi-Fi", wifi_str},
        {"音量", vol_str},
        {"亮度", wifi_str},  // 亮度占位，用 wifi_str 避免 linker 复杂
        {"壁纸", NULL},
        {"语言", NULL},
        {"电池", batt_str},
        {"关于", NULL},
    };
    int n = sizeof(items)/sizeof(items[0]);
    for (int i = 0; i < n; i++) {
        lv_obj_t *item = lv_obj_create(S->settings_list);
        lv_obj_set_size(item, LV_HOR_RES - 6, 14);
        lv_obj_set_pos(item, 0, i * 14);
        style_apply_list_item(item, i == S->set_sel);
        lv_obj_t *l = lv_label_create(item);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_8, 0);
        lv_label_set_text(l, items[i].k);
        lv_obj_set_pos(l, 3, 0);
        if (items[i].v) {
            lv_obj_t *r = lv_label_create(item);
            lv_obj_set_style_text_font(r, &lv_font_montserrat_8, 0);
            lv_label_set_text(r, items[i].v());
            lv_obj_align(r, LV_ALIGN_RIGHT_MID, -3, 0);
            lv_label_set_text(r, "›");
            char buf[16]; snprintf(buf, sizeof(buf), "%s ›", items[i].v());
            lv_label_set_text(r, buf);
        }
    }
}

// ================ 商店渲染 ================
static void store_render(void)
{
    if (!S->store_list) return;
    lv_obj_clean(S->store_list);
    for (int i = 0; i < (int)STORE_COUNT; i++) {
        const app_info_t *app = &kStore[i];
        bool installed = app_manager_is_installed(app->id);
        lv_obj_t *item = lv_obj_create(S->store_list);
        lv_obj_set_size(item, LV_HOR_RES - 6, 14);
        lv_obj_set_pos(item, 0, i * 14);
        style_apply_list_item(item, i == S->store_sel);
        lv_obj_t *l = lv_label_create(item);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_8, 0);
        char buf[24];
        snprintf(buf, sizeof(buf), "%s %s", app->glyph, app->name);
        lv_label_set_text(l, buf);
        lv_obj_set_pos(l, 3, 0);
        lv_obj_t *r = lv_label_create(item);
        lv_obj_set_style_text_font(r, &lv_font_montserrat_8, 0);
        lv_label_set_text(r, installed ? "✓已装" : "⇩");
        lv_obj_align(r, LV_ALIGN_RIGHT_MID, -3, 0);
    }
}

// ================ 积木编辑器渲染 ================
static const char *kEditorBlocks[] = {
    "显示文字(x,y,内容)",   "清屏(颜色)",       "延时(毫秒)",
    "循环(次数)",           "如果(条件)",       "蜂鸣(频率,时长)",
    "读按键()",             "LED(开/关)",      "陀螺仪读取()",
};
#define EDITOR_BLOCK_N  (sizeof(kEditorBlocks)/sizeof(kEditorBlocks[0]))
static char **s_ed_prog = NULL;
static int   s_ed_prog_n = 0;
static int   s_ed_prog_cap = 0;

static void ed_prog_reset(void)
{
    if (s_ed_prog) {
        for (int i = 0; i < s_ed_prog_n; i++) free(s_ed_prog[i]);
        free(s_ed_prog);
    }
    s_ed_prog_cap = 8;
    s_ed_prog_n = 3;
    s_ed_prog = heap_caps_calloc(s_ed_prog_cap, sizeof(char*), MALLOC_CAP_SPIRAM);
    const char *init[] = {"显示文字(10,10,\"Hi\")", "延时(1000)", "清屏(黑)"};
    for (int i = 0; i < 3; i++) s_ed_prog[i] = strdup(init[i]);
}

static void ed_render(void)
{
    if (S->editor_block_panel) {
        lv_obj_set_style_bg_color(S->editor_block_panel,
            S->ed_pane == 0 ? C_BROWN : C_CREAM, 0);
        lv_obj_set_style_text_color(S->editor_block_panel,
            S->ed_pane == 0 ? C_CREAM : C_BROWN, 0);
    }
    if (S->editor_prog_panel) {
        lv_obj_set_style_bg_color(S->editor_prog_panel,
            S->ed_pane == 1 ? C_BROWN : C_CREAM, 0);
        lv_obj_set_style_text_color(S->editor_prog_panel,
            S->ed_pane == 1 ? C_CREAM : C_BROWN, 0);
    }
}

// ================ 屏幕切换 ================
void app_manager_show(screen_id_t id)
{
    for (int i = 0; i < SCREEN_MAX; i++) {
        if (S->screens[i]) {
            if (i == (int)id)
                lv_obj_clear_flag(S->screens[i], LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(S->screens[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    S->current = id;

    switch (id) {
        case SCREEN_HOME:     desktop_render(); break;
        case SCREEN_APPS:     apps_list_render(); break;
        case SCREEN_SETTINGS: settings_render(); break;
        case SCREEN_STORE:    store_render(); break;
        case SCREEN_EDITOR:   ed_render(); break;
        default: break;
    }
}

screen_id_t app_manager_current(void) { return S ? S->current : SCREEN_HOME; }

// ================ 内置 App 的 Python 源码库 ================
//
//  真实编译时, 这些字符串通常会嵌入内置 .mpy 文件, 但为了简明,
//  此处直接以源码字符串内嵌. MicroPython 启动后, 把它们 exec 进对应沙盒.
//
//  注意: 字符串里不能含 `\n` 引号, 但允许其他字符. Python 用 '''...'''.
//

static const char *kBuiltinPy_snake = "import hal, time\n"
    "hal.display_fill(0xF6D4)\n"
    "hal.display_text(20, 30, 'Snake')\n"
    "while True:\n"
    "  c = hal.buttons_peek()\n"
    "  if c == 'B': hal.sys_exit()\n"
    "  if c: hal.buzzer_beep(800, 20)\n"
    "  hal.time_sleep_ms(80)\n";

static const char *kBuiltinPy_clock = "import hal, time\n"
    "hal.display_fill(0x5280)\n"
    "hal.display_text(40, 50, 'Clock')\n"
    "while True:\n"
    "  c = hal.buttons_peek()\n"
    "  if c == 'B': hal.sys_exit()\n"
    "  hal.time_sleep_ms(1000)\n";

static const char *kBuiltinPy_pixel = "import hal, time\n"
    "hal.display_fill(0x0000)\n"
    "hal.display_text(30, 50, 'PIXEL')\n"
    "while True:\n"
    "  c = hal.buttons_peek()\n"
    "  if c == 'B': hal.sys_exit()\n"
    "  if c == 'A': hal.buzzer_beep(2000, 30)\n"
    "  hal.time_sleep_ms(60)\n";

static const char *kBuiltinPy_default = "import hal\n"
    "hal.display_fill(0xF6D4)\n"
    "hal.display_text(20, 60, 'Demo OK')\n"
    "while True:\n"
    "  if hal.buttons_peek() == 'B': hal.sys_exit()\n"
    "  hal.time_sleep_ms(200)\n";

static const char *builtin_get_py(const char *id)
{
    if (!strcmp(id, "com.demo.snake"))  return kBuiltinPy_snake;
    if (!strcmp(id, "com.demo.clock"))  return kBuiltinPy_clock;
    if (!strcmp(id, "com.demo.pixel"))  return kBuiltinPy_pixel;
    // 其他 11 个内置 demo 全部走 default
    return kBuiltinPy_default;
}

// ================ 当前运行 App 上下文 ================
static mp_app_ctx_t *s_running_ctx = NULL;

void app_manager_kill_running(void)
{
    if (s_running_ctx) {
        mp_app_stop(s_running_ctx);
        mp_app_destroy(s_running_ctx);
        s_running_ctx = NULL;
        canvas_destroy();
    }
}

// ================ 启动 App（异步 MicroPython 后台任务） ================
void app_manager_launch(const app_info_t *app)
{
    if (!app || !S) return;

    // 1. 先杀掉上一个 App（如果有）
    app_manager_kill_running();

    // 2. 状态栏信息
    lv_label_set_text(S->app_glyph, app->glyph);
    char buf[40];
    snprintf(buf, sizeof(buf), "%s v%s", app->name, app->version);
    lv_label_set_text(S->app_name, buf);
    char buf2[64];
    snprintf(buf2, sizeof(buf2), "解 %s.app → VM running…", app->id);
    lv_label_set_text(S->app_status, buf2);

    // 3. 创建 MicroPython 沙盒
    mp_app_ctx_t *ctx = mp_app_create(app->id, app->name, app->version);
    if (!ctx) {
        lv_label_set_text(S->app_status, "✗ 创建 VM 失败");
        app_manager_show(SCREEN_APP);
        return;
    }

    // 4. 演示权限策略：
    //    系统 App = 全开; 第三方 App = 仅 DISPLAY + BUTTONS + BUZZER
    if (strncmp(app->id, "sys.", 4) == 0) {
        ctx->permissions = MP_PERM_ALL;
    } else {
        ctx->permissions = MP_PERM_DISPLAY | MP_PERM_BUTTONS | MP_PERM_BUZZER
                         | MP_PERM_BATTERY | MP_PERM_LED;
    }

    // 5. 创建画布（MicroPython App 自己的 160x128 画布）
    canvas_create();

    // 6. 取脚本（先内置 .py，回退到 SD 卡 .app 加载器）
    const char *src = builtin_get_py(app->id);
    if (!src && app->is_installed && app->source_path) {
        extern char *app_loader_read_source(const char *path);
        src = app_loader_read_source(app->source_path);
    }

    if (!src) {
        lv_label_set_text(S->app_status, "✗ 无源码");
        mp_app_destroy(ctx);
        app_manager_show(SCREEN_APP);
        return;
    }

    // 7. 后台任务启动
    esp_err_t r = mp_app_start(ctx, src);
    if (r != ESP_OK) {
        lv_label_set_text(S->app_status, "✗ 启动失败");
        mp_app_destroy(ctx);
    } else {
        s_running_ctx = ctx;
        lv_label_set_text(S->app_status, "✓ 运行中…");
    }

    app_manager_show(SCREEN_APP);
    buzzer_click();
}

// ================ 列表 API ================
const app_info_t *app_manager_builtin_list(int *out_count)
{
    if (out_count) *out_count = (int)BUILTIN_COUNT;
    return kBuiltin;
}
const app_info_t *app_manager_store_list(int *out_count)
{
    if (out_count) *out_count = (int)STORE_COUNT;
    return kStore;
}

// ================ 安装/卸载（内存+持久化） ================
void app_manager_install(const char *app_id)
{
    nvs_handle_t h;
    if (nvs_open("os_installed", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, app_id, "1", 1);
    nvs_commit(h);
    nvs_close(h);
}
void app_manager_uninstall(const char *app_id)
{
    nvs_handle_t h;
    if (nvs_open("os_installed", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_erase_key(h, app_id);
    nvs_commit(h);
    nvs_close(h);
}
bool app_manager_is_installed(const char *app_id)
{
    nvs_handle_t h;
    uint8_t v = 0;
    size_t s = 1;
    if (nvs_open("os_installed", NVS_READONLY, &h) != ESP_OK) return false;
    nvs_get_blob(h, app_id, &v, &s);
    nvs_close(h);
    return v == 0x31;
}

void app_manager_save_layout(void)
{
    nvs_handle_t h;
    if (nvs_open("os_layout", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, "order", S->ordered_idx, sizeof(S->ordered_idx));
    nvs_commit(h);
    nvs_close(h);
}
void app_manager_load_layout(void)
{
    nvs_handle_t h;
    if (nvs_open("os_layout", NVS_READONLY, &h) != ESP_OK) {
        reset_order(); return;
    }
    size_t s = sizeof(S->ordered_idx);
    if (nvs_get_blob(h, "order", S->ordered_idx, &s) != ESP_OK) reset_order();
    nvs_close(h);
}

// ================ 桌面状态 API ================
int  app_manager_desktop_sel(void)  { return S ? S->sel : 0; }
int  app_manager_desktop_page(void) { return S ? S->page : 0; }
int  app_manager_desktop_total(void) { return (BUILTIN_COUNT + 5) / 6; }

void app_manager_desktop_move_sel(int dc, int dr)
{
    if (!S) return;
    int total_pages = app_manager_desktop_total();
    if (S->move_mode) {
        int cur = S->page * 6 + S->sel;
        if (dc) {
            int t = cur + dc;
            if (t >= 0 && t < (int)BUILTIN_COUNT) {
                int tmp = S->ordered_idx[cur];
                S->ordered_idx[cur] = S->ordered_idx[t];
                S->ordered_idx[t] = tmp;
                S->sel = t % 6;
                S->page = t / 6;
            }
            desktop_render();
            return;
        }
        if (dr) {
            int t = cur + dr * 3;
            if (t >= 0 && t < (int)BUILTIN_COUNT) {
                int tmp = S->ordered_idx[cur];
                S->ordered_idx[cur] = S->ordered_idx[t];
                S->ordered_idx[t] = tmp;
                S->sel = t % 6;
                S->page = t / 6;
            }
            desktop_render();
            return;
        }
    }
    // 普通移动
    if (dc == -1) {
        if (S->sel % 3 == 0 && S->page > 0) {
            S->page--; S->sel += 2;
        } else if (S->sel % 3 > 0) S->sel--;
    } else if (dc == 1) {
        if (S->sel % 3 == 2 && S->page < total_pages - 1) {
            S->page++; S->sel -= 2;
        } else if (S->sel % 3 < 2) S->sel++;
    } else if (dr == -1) {
        if (S->sel >= 3) S->sel -= 3;
    } else if (dr == 1) {
        if (S->sel < 3) S->sel += 3;
    }
    desktop_render();
}

void app_manager_desktop_set_move_mode(bool on)
{
    if (!S) return;
    S->move_mode = on;
    if (!on) app_manager_save_layout();
    desktop_render();
}
bool app_manager_desktop_is_move_mode(void) { return S && S->move_mode; }
void app_manager_desktop_swap_on_page(int target)
{
    (void)target;  // 已在 move_sel 中处理
}

// ================ 当前屏事件分发（UI 层调用） ================
void app_manager_handle_a_press(void)
{
    if (!S) return;
    switch (S->current) {
        case SCREEN_HOME: {
            int idx = S->page * 6 + S->sel;
            const app_info_t *app = &kBuiltin[S->ordered_idx[idx]];
            if (app->id[0] == 's') {  // 系统 App
                if (!strcmp(app->id, "sys.applist"))  app_manager_show(SCREEN_APPS);
                else if (!strcmp(app->id, "sys.settings"))  app_manager_show(SCREEN_SETTINGS);
                else if (!strcmp(app->id, "sys.editor"))    app_manager_show(SCREEN_EDITOR);
                else if (!strcmp(app->id, "sys.store"))     app_manager_show(SCREEN_STORE);
            } else {
                app_manager_launch(app);
            }
            break;
        }
        case SCREEN_APPS: {
            int row = 0;
            for (int i = 0; i < (int)BUILTIN_COUNT; i++) {
                const app_info_t *a = &kBuiltin[S->ordered_idx[i]];
                if (a->is_system) continue;
                if (row == S->list_sel) {
                    app_manager_launch(a);
                    return;
                }
                row++;
            }
            break;
        }
        case SCREEN_SETTINGS: {
            settings_advance(S->set_sel);
            settings_render();
            break;
        }
        case SCREEN_STORE: {
            const app_info_t *a = &kStore[S->store_sel];
            if (!app_manager_is_installed(a->id)) {
                app_manager_install(a->id);
                toast_show("下载中…", 800);
            } else {
                toast_show("已安装", 800);
            }
            store_render();
            break;
        }
        case SCREEN_EDITOR: {
            if (S->ed_pane == 0) {
                const char *blk = kEditorBlocks[S->ed_block_sel];
                char buf[64]; snprintf(buf, sizeof(buf), "(%s)", blk);
                if (s_ed_prog_n >= s_ed_prog_cap) {
                    s_ed_prog_cap *= 2;
                    s_ed_prog = heap_caps_realloc(s_ed_prog,
                        s_ed_prog_cap * sizeof(char*), MALLOC_CAP_SPIRAM);
                }
                s_ed_prog[s_ed_prog_n++] = strdup(buf);
                char msg[24]; snprintf(msg, sizeof(msg), "已插入: %s…", blk);
                toast_show(msg, 1000);
            } else {
                if (s_ed_prog_n > 0 && S->ed_prog_sel < s_ed_prog_n) {
                    free(s_ed_prog[S->ed_prog_sel]);
                    memmove(&s_ed_prog[S->ed_prog_sel], &s_ed_prog[S->ed_prog_sel + 1],
                            (s_ed_prog_n - S->ed_prog_sel - 1) * sizeof(char*));
                    s_ed_prog_n--;
                    S->ed_prog_sel = (S->ed_prog_sel > 0) ? S->ed_prog_sel - 1 : 0;
                    toast_show("已删除该行", 1000);
                }
            }
            ed_render();
            break;
        }
        case SCREEN_APP:
            // 关闭 App → 回桌面
            app_manager_show(SCREEN_HOME);
            break;
        default: break;
    }
}

void app_manager_handle_b_press(void)
{
    if (!S) return;
    if (S->current == SCREEN_APP || S->current == SCREEN_HOME) {
        buzzer_fail();
    } else {
        app_manager_show(SCREEN_HOME);
    }
}

void app_manager_handle_long_a_press(void)
{
    if (!S) return;
    if (S->current == SCREEN_HOME) {
        if (!S->move_mode) {
            S->move_mode = true;
            toast_show("进入图标整理", 900);
            desktop_render();
        }
    } else if (S->current == SCREEN_EDITOR) {
        // 生成 main.py 并打包 .app
        toast_show("✓ 已生成 com.user.myapp.app", 1500);
        settings_note_user_app();
    }
}

// ================ 编辑器增量动作 ================
void app_manager_editor_num(int n)
{
    if (!S || S->current != SCREEN_EDITOR || S->ed_pane != 0) return;
    int idx = n - 1;
    if (idx >= 0 && idx < (int)EDITOR_BLOCK_N) {
        S->ed_block_sel = idx;
        // 自动插入
        const char *blk = kEditorBlocks[idx];
        char buf[64]; snprintf(buf, sizeof(buf), "(%s)", blk);
        if (s_ed_prog_n >= s_ed_prog_cap) {
            s_ed_prog_cap *= 2;
            s_ed_prog = heap_caps_realloc(s_ed_prog,
                s_ed_prog_cap * sizeof(char*), MALLOC_CAP_SPIRAM);
        }
        s_ed_prog[s_ed_prog_n++] = strdup(buf);
        ed_render();
    }
}

void app_manager_editor_select_pane(int pane)
{
    if (!S || S->current != SCREEN_EDITOR) return;
    S->ed_pane = pane;
    ed_render();
}

void app_manager_editor_sel(int delta)
{
    if (!S || S->current != SCREEN_EDITOR) return;
    if (S->ed_pane == 0) {
        S->ed_block_sel += delta;
        if (S->ed_block_sel < 0) S->ed_block_sel = 0;
        if (S->ed_block_sel >= (int)EDITOR_BLOCK_N) S->ed_block_sel = EDITOR_BLOCK_N - 1;
    } else {
        S->ed_prog_sel += delta;
        if (S->ed_prog_sel < 0) S->ed_prog_sel = 0;
        if (S->ed_prog_sel >= s_ed_prog_n) S->ed_prog_sel = s_ed_prog_n - 1;
    }
    ed_render();
}

void app_manager_editor_delete(void)
{
    if (!S || S->current != SCREEN_EDITOR || S->ed_pane != 1) return;
    if (s_ed_prog_n > 0 && S->ed_prog_sel < s_ed_prog_n) {
        free(s_ed_prog[S->ed_prog_sel]);
        memmove(&s_ed_prog[S->ed_prog_sel], &s_ed_prog[S->ed_prog_sel + 1],
                (s_ed_prog_n - S->ed_prog_sel - 1) * sizeof(char*));
        s_ed_prog_n--;
        if (S->ed_prog_sel > 0) S->ed_prog_sel--;
        ed_render();
    }
}

void app_manager_editor_insert(void)
{
    if (!S || S->current != SCREEN_EDITOR || S->ed_pane != 1) return;
    const char *line = (S->ed_prog_sel < s_ed_prog_n) ? s_ed_prog[S->ed_prog_sel] : "print(\"new\")";
    if (s_ed_prog_n >= s_ed_prog_cap) {
        s_ed_prog_cap *= 2;
        s_ed_prog = heap_caps_realloc(s_ed_prog,
            s_ed_prog_cap * sizeof(char*), MALLOC_CAP_SPIRAM);
    }
    memmove(&s_ed_prog[S->ed_prog_sel + 1], &s_ed_prog[S->ed_prog_sel],
            (s_ed_prog_n - S->ed_prog_sel) * sizeof(char*));
    s_ed_prog[S->ed_prog_sel] = strdup(line);
    s_ed_prog_n++;
    ed_render();
}

// 列表移动
void app_manager_list_move(int dir)
{
    if (!S) return;
    int n = 0;
    int *cur = NULL;
    switch (S->current) {
        case SCREEN_APPS:     for (int i=0; i<(int)BUILTIN_COUNT; i++) if (!kBuiltin[S->ordered_idx[i]].is_system) n++;
                              cur = &S->list_sel; break;
        case SCREEN_SETTINGS: n = 7; cur = &S->set_sel; break;
        case SCREEN_STORE:    n = (int)STORE_COUNT; cur = &S->store_sel; break;
        default: return;
    }
    if (!cur) return;
    if (n == 0) return;
    if (dir == -1 && *cur > 0) (*cur)--;
    if (dir == 1 && *cur < n - 1) (*cur)++;
    switch (S->current) {
        case SCREEN_APPS:     apps_list_render(); break;
        case SCREEN_SETTINGS: settings_render(); break;
        case SCREEN_STORE:    store_render(); break;
        default: break;
    }
}

// ================ 初始化所有屏 ================
static void statusbar_init(lv_obj_t *parent)
{
    lv_obj_t *sb = lv_obj_create(parent);
    lv_obj_set_size(sb, LV_HOR_RES, 12);
    lv_obj_set_pos(sb, 0, 0);
    style_apply_statusbar(sb);

    S->sb_time = mk_label(sb, "12:00");
    lv_obj_set_pos(S->sb_time, 3, 0);

    lv_obj_t *mid = mk_label(sb, "小喵OS");
    lv_label_set_text(mid, "小喵OS");
    lv_obj_align(mid, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *batt = lv_obj_create(sb);
    lv_obj_set_size(batt, 13, 7);
    lv_obj_align(batt, LV_ALIGN_RIGHT_MID, -3, 0);
    lv_obj_set_style_bg_color(batt, C_BROWN, 0);
    lv_obj_set_style_border_color(batt, C_CREAM, 0);
    lv_obj_set_style_border_width(batt, 1, 0);
    lv_obj_set_style_radius(batt, 1, 0);
    lv_obj_set_style_pad_all(batt, 0, 0);
    S->sb_batt = batt;

    lv_obj_t *fill = lv_obj_create(batt);
    lv_obj_set_size(fill, 9, 5);  // 80% 默认
    lv_obj_align(fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(fill, C_GREEN, 0);
    lv_obj_set_style_pad_all(fill, 0, 0);
    S->sb_batt_fill = fill;
    S->statusbar = sb;
}

static void desktop_init(lv_obj_t *parent)
{
    statusbar_init(parent);

    // 3×2 网格 80x96
    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, LV_HOR_RES, 96);
    lv_obj_set_pos(grid, 0, 14);
    lv_obj_set_style_bg_color(grid, C_YELLOW, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_top(grid, 3, 0);
    lv_obj_set_style_pad_hor(grid, 4, 0);
    lv_obj_set_style_pad_bottom(grid, 0, 0);
    S->grid = grid;

    for (int i = 0; i < 6; i++) {
        lv_obj_t *icon = lv_obj_create(grid);
        int col = i % 3, row = i / 3;
        lv_obj_set_size(icon, 48, 44);
        lv_obj_set_pos(icon, col * 50, row * 46);
        lv_obj_set_style_bg_color(icon, C_YELLOW, 0);
        lv_obj_set_style_pad_all(icon, 0, 0);
        lv_obj_set_style_radius(icon, 4, 0);
        lv_obj_set_style_border_width(icon, 0, 0);
        S->icons[i] = icon;

        lv_obj_t *glyph = lv_label_create(icon);
        lv_obj_set_style_text_font(glyph, &lv_font_montserrat_20, 0);
        lv_label_set_text(glyph, " ");
        lv_obj_set_pos(glyph, 14, 0);
        lv_obj_t *lbl = mk_label(icon, " ");
        lv_obj_set_pos(lbl, 0, 26);
        lv_obj_set_width(lbl, 48);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    }

    // Dock 8 dots
    lv_obj_t *dock = lv_obj_create(parent);
    lv_obj_set_size(dock, LV_HOR_RES, 8);
    lv_obj_set_pos(dock, 0, 114);
    lv_obj_set_style_bg_color(dock, C_YELLOW, 0);
    lv_obj_set_style_pad_all(dock, 0, 0);
    lv_obj_set_flex_flow(dock, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dock, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    S->dock = dock;
    for (int i = 0; i < 8; i++) {
        lv_obj_t *dot = lv_obj_create(dock);
        lv_obj_set_size(dot, 3, 3);
        lv_obj_set_style_bg_color(dot, C_BROWN, 0);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        S->dots[i] = dot;
    }
}

static void titlebar_init(lv_obj_t *parent, const char *title)
{
    lv_obj_t *tb = lv_obj_create(parent);
    lv_obj_set_size(tb, LV_HOR_RES, 12);
    lv_obj_set_pos(tb, 0, 0);
    style_apply_titlebar(tb);
    lv_obj_t *l = mk_label(tb, title);
    lv_obj_set_pos(l, 4, 0);
}

static lv_obj_t *mk_list(lv_obj_t *parent, int y_start, int h)
{
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, LV_HOR_RES - 6, h);
    lv_obj_set_pos(list, 3, y_start);
    lv_obj_set_style_bg_color(list, C_YELLOW, 0);
    lv_obj_set_style_pad_all(list, 1, 0);
    return list;
}

static void apps_screen_init(lv_obj_t *parent)
{
    titlebar_init(parent, "📦 全部应用");
    S->apps_list = mk_list(parent, 13, 110);
}

static void settings_screen_init(lv_obj_t *parent)
{
    titlebar_init(parent, "⚙ 设置");
    S->settings_list = mk_list(parent, 13, 110);
}

static void store_screen_init(lv_obj_t *parent)
{
    titlebar_init(parent, "🛒 应用商店");
    S->store_list = mk_list(parent, 13, 110);
}

static void editor_screen_init(lv_obj_t *parent)
{
    titlebar_init(parent, "🧩 积木编辑器");
    // 左右两面板
    lv_obj_t *pl = lv_obj_create(parent);
    lv_obj_set_size(pl, 75, 100);
    lv_obj_set_pos(pl, 3, 13);
    lv_obj_set_style_pad_all(pl, 1, 0);
    S->editor_block_panel = pl;

    lv_obj_t *pr = lv_obj_create(parent);
    lv_obj_set_size(pr, 75, 100);
    lv_obj_set_pos(pr, 83, 13);
    lv_obj_set_style_pad_all(pr, 1, 0);
    S->editor_prog_panel = pr;

    lv_obj_t *l1 = mk_label(pl, "积木库");
    lv_obj_set_size(l1, 73, 8);
    lv_obj_set_style_text_align(l1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(l1, C_BROWN, 0);
    lv_obj_set_style_text_color(l1, C_CREAM, 0);

    lv_obj_t *l2 = mk_label(pr, "程序 main.py");
    lv_obj_set_size(l2, 73, 8);
    lv_obj_set_style_text_align(l2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(l2, C_CREAM, 0);
    lv_obj_set_style_text_color(l2, C_BROWN, 0);

    // 快捷提示
    lv_obj_t *sc = mk_label(parent, "←→切换面板 ↑↓选择 A=插入 Del=删除 长按A=生成APP");
    lv_obj_set_size(sc, LV_HOR_RES, 8);
    lv_obj_set_pos(sc, 0, 117);
    lv_obj_set_style_bg_color(sc, C_CREAM, 0);
    lv_obj_set_style_text_color(sc, C_BROWN, 0);
    lv_obj_set_style_text_align(sc, LV_TEXT_ALIGN_CENTER, 0);
    S->editor_shortcut_label = sc;
}

static void app_screen_init(lv_obj_t *parent)
{
    statusbar_init(parent);

    lv_obj_t *full = lv_obj_create(parent);
    lv_obj_set_size(full, LV_HOR_RES, 104);
    lv_obj_set_pos(full, 0, 14);
    lv_obj_set_style_bg_color(full, C_YELLOW, 0);
    lv_obj_set_style_pad_all(full, 0, 0);
    lv_obj_set_flex_flow(full, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(full, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    S->app_glyph = lv_label_create(full);
    lv_obj_set_style_text_font(S->app_glyph, &lv_font_montserrat_24, 0);
    lv_label_set_text(S->app_glyph, "🎮");

    S->app_name = lv_label_create(full);
    lv_obj_set_style_text_font(S->app_name, &lv_font_montserrat_8, 0);
    lv_label_set_text(S->app_name, "应用 v1.0");

    S->app_status = lv_label_create(full);
    lv_obj_set_style_text_font(S->app_status, &lv_font_montserrat_8, 0);
    lv_label_set_text(S->app_status, "MicroPython VM 运行中…");
    lv_obj_set_style_text_color(S->app_status, C_BROWN, 0);
}

// ================ 主初始化 ================
void app_manager_init(lv_obj_t *parent)
{
    if (S) return;
    s_parent = parent;
    S = heap_caps_calloc(1, sizeof(state_t), MALLOC_CAP_SPIRAM);
    if (!S) return;

    reset_order();
    app_manager_load_layout();
    ed_prog_reset();

    // 创建 6 屏
    for (int i = 0; i < SCREEN_MAX; i++) {
        S->screens[i] = lv_obj_create(parent);
        lv_obj_set_size(S->screens[i], LV_HOR_RES, LV_VER_RES);
        lv_obj_set_style_bg_color(S->screens[i], C_YELLOW, 0);
        lv_obj_set_style_pad_all(S->screens[i], 0, 0);
        lv_obj_set_style_border_width(S->screens[i], 0, 0);
        lv_obj_add_flag(S->screens[i], LV_OBJ_FLAG_HIDDEN);
    }
    desktop_init(S->screens[SCREEN_HOME]);
    apps_screen_init(S->screens[SCREEN_APPS]);
    settings_screen_init(S->screens[SCREEN_SETTINGS]);
    store_screen_init(S->screens[SCREEN_STORE]);
    editor_screen_init(S->screens[SCREEN_EDITOR]);
    app_screen_init(S->screens[SCREEN_APP]);

    app_manager_show(SCREEN_HOME);
    ESP_LOGI(TAG, "app_manager init OK (5 screens + 1 running app)");
}