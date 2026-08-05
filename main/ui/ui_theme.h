/*
 * ui_theme.h — XiaoMiao OS shared color palette & layout constants
 * Mirrors the approved HTML UI simulator (xiaomiao-ui-sim.html)
 */
#pragma once

#include "lvgl.h"

/* Original firmware color palette */
#define THEME_YELLOW  0xF6D34A
#define THEME_BLACK   0x1B1713
#define THEME_BROWN   0x5C4220
#define THEME_RED     0xE64B3C
#define THEME_CREAM   0xFFF3B0
#define THEME_GREEN   0x2DD466
#define THEME_BLUE    0x3A5C8A

/* Screen physical resolution (landscape, rotated 90°) */
#define UI_SCREEN_W   160
#define UI_SCREEN_H   128

/* Layout bands (px), matching HTML sim */
#define UI_STATUSBAR_H  12
#define UI_TITLEBAR_H   12
#define UI_DOCK_H       8

/* Desktop grid: 3 columns x 2 rows = 6 icons per page (per user spec) */
#define UI_GRID_COLS    3
#define UI_GRID_ROWS    2
#define UI_ICONS_PER_PAGE (UI_GRID_COLS * UI_GRID_ROWS)

static inline lv_color_t th_color(uint32_t hex) { return lv_color_hex(hex); }