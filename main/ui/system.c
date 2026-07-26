/*
 * system.c - 系统状态管理
 */
#include "ui/system.h"

system_status_t g_system_status = {
    .battery_percent = 100,
    .battery_voltage = 0.0f,
    .charging = false,
    .sd_mounted = false,
    .wifi_connected = false,
    .uptime_seconds = 0,
};
