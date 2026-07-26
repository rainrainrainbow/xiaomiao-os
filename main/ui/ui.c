/*
 * ui.c - UI 框架初始化
 */
#include "esp_log.h"
#include "ui/ui.h"

static const char *TAG = "ui";

void ui_init_all(void)
{
    ESP_LOGI(TAG, "Initializing UI framework...");

    ui_theme_init();
    page_manager_init();

    ESP_LOGI(TAG, "UI ready");
}
