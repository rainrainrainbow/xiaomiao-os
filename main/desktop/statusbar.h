#pragma once
#include "lvgl.h"

void statusbar_create(void);
void statusbar_update(void);
void statusbar_set_time(const char *time_str);
void statusbar_set_battery(int percent);
