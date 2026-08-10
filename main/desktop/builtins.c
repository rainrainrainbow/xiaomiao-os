// ================ builtins.c - 14 个内置 App 占位实现 ================

#include "builtins.h"
#include "core/app_manager.h"
#include "ui/theme.h"
#include "hal/buzzer.h"
#include "hal/mpu6050.h"
#include "hal/led_motor.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "builtins";

// 已安装 App 的演示循环（每帧切换一小段动画 / 数字）
void builtins_run_step(const app_info_t *app, uint32_t tick_ms)
{
    if (!app) return;
    ESP_LOGD(TAG, "run %s tick=%u", app->id, tick_ms);
    // 不同 App 可绑定不同 demo
    if (!strcmp(app->id, "com.demo.snake")) {
        // 贪吃蛇: 闪背景灯
        static int phase = 0;
        if ((tick_ms / 200) % 4 == 0) buzzer_beep(800, 50);
        (void)phase;
    } else if (!strcmp(app->id, "com.demo.music")) {
        static uint32_t last = 0;
        if (tick_ms - last > 500) {
            buzzer_beep(1000 + ((tick_ms / 200) % 12) * 100, 80);
            last = tick_ms;
        }
    } else if (!strcmp(app->id, "com.demo.pixel")) {
        // 像素鸟: 用 MPU6050 翻动控制（demo）
        static uint32_t last = 0;
        if (tick_ms - last > 100 && mpu6050_is_ready()) {
            mpu6050_data_t d;
            if (mpu6050_read(&d) == ESP_OK && d.gy < -200) {
                buzzer_beep(2000, 30);
            }
            last = tick_ms;
        }
    } else if (!strcmp(app->id, "com.demo.timer")) {
        // 秒表: 蜂鸣器计数
        static uint32_t last = 0;
        if (tick_ms - last > 1000) {
            buzzer_click();
            last = tick_ms;
        }
    } else if (!strcmp(app->id, "com.demo.clock")) {
        // 时钟: 小时点 beep
        static uint32_t last = 0;
        if (tick_ms - last > 3600 * 1000) {
            buzzer_success();
            last = tick_ms;
        }
    }
    gd32_set_led(2);  // LED 呼吸表示在运行
}

void builtins_register(void)
{
    // 占位实现: 14 个 App 都已硬编码在 app_manager.c 的 kBuiltin
    // 这个函数留给后续加载自定义 .app 扩展
    ESP_LOGI(TAG, "builtins registered (%d)", 14);
}