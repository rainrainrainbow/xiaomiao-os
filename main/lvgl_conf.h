// ================ lvgl_conf.h - 小喵 OS 专用 LVGL 9.5 配置 ================
//
//  适配 160x128 ST7735 极小彩屏, 内存尽量省.
//
//  详见: https://docs.lvgl.io/9.5/CHANGELOG.html

#ifndef LV_CONF_H
#define LV_CONF_H

// 颜色深度（与 ST7735 RGB565 一致）
#define LV_COLOR_DEPTH 16

// 渲染模式（CPU 全软渲染 + 三缓冲）
#define LV_DRAW_SW_COMPLEX_DEFAULT 1
#define LV_USE_PARALLEL              0

// 字号 (启用 app_manager.c 用到的全部)
#define LV_FONT_MONTSERRAT_8         1
#define LV_FONT_MONTSERRAT_10        0
#define LV_FONT_MONTSERRAT_12        0
#define LV_FONT_MONTSERRAT_14        1
#define LV_FONT_MONTSERRAT_16        0
#define LV_FONT_MONTSERRAT_18        0
#define LV_FONT_MONTSERRAT_20        1
#define LV_FONT_MONTSERRAT_22        0
#define LV_FONT_MONTSERRAT_24        1
#define LV_FONT_MONTSERRAT_26        0
#define LV_FONT_MONTSERRAT_28        0
#define LV_FONT_MONTSERRAT_30        0
#define LV_FONT_MONTSERRAT_32        0
#define LV_FONT_MONTSERRAT_34        0
#define LV_FONT_MONTSERRAT_36        0
#define LV_FONT_MONTSERRAT_38        0
#define LV_FONT_MONTSERRAT_40        0
#define LV_FONT_MONTSERRAT_42        0
#define LV_FONT_MONTSERRAT_44        0
#define LV_FONT_MONTSERRAT_46        0
#define LV_FONT_MONTSERRAT_48        0
#define LV_FONT_DEFAULT              &lv_font_montserrat_14

// 主题
#define LV_USE_THEME_DEFAULT         1
#define LV_THEME_DEFAULT_DARK        0
#define LV_THEME_DEFAULT_GROW        0
#define LV_THEME_DEFAULT_PRIMARY_COLOR lv_color_hex(0xF6D34A)

#define LV_USE_THEME_MONO            0
#define LV_USE_THEME_CUSTOM          0

// 控件：只启用必需的
#define LV_USE_BTN                   1
#define LV_USE_LABEL                 1
#define LV_USE_SLIDER                1
#define LV_USE_ARC                   1   // spinner 需要
#define LV_USE_SWITCH                1
#define LV_USE_LIST                  1
#define LV_USE_DROPDOWN              1   // calendar header 需要
#define LV_USE_TABLE                 0
#define LV_USE_TABVIEW               0
#define LV_USE_WIN                   0
#define LV_USE_IMG                   1
#define LV_USE_IMGBTN                0
#define LV_USE_CANVAS                1
#define LV_USE_BAR                   1
#define LV_USE_LINE                  1
#define LV_USE_CHECKBOX              0
#define LV_USE_ROLLER                0
#define LV_USE_TEXTAREA              1
#define LV_USE_SPINBOX               0
// 关闭不需要的 widget（避免连锁要求更多 widget）
#define LV_USE_CALENDAR              0
#define LV_USE_SPINNER               0
#define LV_USE_LED                   0
#define LV_USE_METER                 0
#define LV_USE_ANIMIMG               0
#define LV_USE_QRCODE                0

// 布局
#define LV_USE_FLEX                  1
#define LV_USE_GRID                  1

// 内存
#define LV_MEM_SIZE                  (24 * 1024)   // 24KB (足够 160x128)
#define LV_MEM_POOL_INCLUDE          "esp_heap_caps.h"
#define LV_MEM_POOL_ALLOC            malloc
#define LV_MEM_POOL_FREE             free
#define LV_MEM_POOL_REALLOC          realloc

// 定时器
#define LV_DEF_REFR_PERIOD           33   // ~30 FPS

// 日志
#define LV_USE_LOG                   0

// 性能
#define LV_DRAW_BUF_STRIDE_ALIGN     1
#define LV_DRAW_LINE_WIDTH_1         1

// 字节序
#define LV_USE_BIG_ENDIAN            0

#endif // LV_CONF_H