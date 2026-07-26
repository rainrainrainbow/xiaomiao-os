#pragma once
/*
 * system.h - 系统全局数据结构
 */
#include <stdbool.h>
#include <stdint.h>

#define MAX_SSID_LEN  32
#define MAX_PWD_LEN   64

/* 系统设置 */
typedef struct {
    uint8_t  brightness;       /* 0-255 */
    uint8_t  volume;           /* 0-100 */
    uint8_t  language;         /* 0=中文 1=English */
    char     wifi_ssid[MAX_SSID_LEN];
    char     wifi_password[MAX_PWD_LEN];
    uint16_t auto_sleep;       /* 秒 */
    bool     debug_mode;
} system_settings_t;

/* 系统状态 */
typedef struct {
    uint8_t  battery_percent;
    float    battery_voltage;
    bool     charging;
    bool     sd_mounted;
    bool     wifi_connected;
    uint32_t uptime_seconds;
} system_status_t;

extern system_status_t g_system_status;
