/*
 * XiaoMiao OS - MicroPython Desktop System
 *
 * ESP32-WROVER-B + ST7735 160x128 TFT + MicroSD + 6-key keypad.
 * Includes return-to-loader integration.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdkconfig.h"

#include "lvgl_port.h"
#include "app_manager.h"
#include "micropython_runtime.h"
#include "config_manager.h"
#include "power_manager.h"
#include "ui_main.h"

static const char *TAG = "xiaomiao-os";

/* ── Hardware Constants ────────────────────────────────────────────────── */
#define LCD_HOST            SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ  (60 * 1000 * 1000)
#define LCD_NATIVE_H_RES    128
#define LCD_NATIVE_V_RES    160
#define LCD_H_RES           160
#define LCD_V_RES           128
#define LCD_DRAW_BUF_LINES  LCD_V_RES
#define LCD_DPI             60
#define PIN_LCD_SCLK   GPIO_NUM_18
#define PIN_LCD_MOSI   GPIO_NUM_23
#define PIN_LCD_MISO   GPIO_NUM_19
#define PIN_LCD_CS     GPIO_NUM_5
#define PIN_LCD_DC     GPIO_NUM_4
#define PIN_SD_CS      GPIO_NUM_22

// I2C
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SCL_IO   15
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_FREQ_HZ  100000

// Buzzer
#define BUZZER_GPIO         GPIO_NUM_14
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_CHANNEL        LEDC_CHANNEL_0

#define BUTTON_ACTIVE_LEVEL  0
#define BUTTON_DEBOUNCE_MS   25

#define LVGL_TICK_PERIOD_MS  1
#define LVGL_TASK_STACK      (10 * 1024)
#define LVGL_TASK_PRIORITY   5

/* ST7735 registers */
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1
#define MADCTL_MX       0x40
#define MADCTL_MY       0x80
#define MADCTL_MV       0x20
#define MADCTL_RGB      0x00

/* ── Globals ───────────────────────────────────────────────────────────── */
static esp_lcd_panel_io_handle_t s_lcd_io = NULL;
static sdmmc_card_t *s_sd_card = NULL;

/* ── Return to Loader ──────────────────────────────────────────────────── */
static void return_to_loader_setup(void)
{
    const esp_partition_t *cur = esp_ota_get_running_partition();
    if (!cur || cur->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_0) {
        ESP_LOGI(TAG, "Not running from OTA partition, skip loader return setup");
        return;
    }
    
    const esp_partition_t *fac = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (fac) {
        esp_ota_set_boot_partition(fac);
        ESP_LOGI(TAG, "Return to loader configured (factory partition set)");
    } else {
        ESP_LOGW(TAG, "Factory partition not found, cannot return to loader");
    }
}

/* ── LCD ───────────────────────────────────────────────────────────────── */
static void st7735_tx(esp_lcd_panel_io_handle_t io, int cmd,
                      const void *param, size_t len)
{
    esp_lcd_panel_io_tx_param(io, cmd, param, len);
}

static void st7735_delay(uint32_t ms) { 
    vTaskDelay(pdMS_TO_TICKS(ms)); 
}

static void st7735_init(esp_lcd_panel_io_handle_t io)
{
    const uint8_t frmctr[]  = {0x01,0x2C,0x2D};
    const uint8_t frmctr3[] = {0x01,0x2C,0x2D,0x01,0x2C,0x2D};
    const uint8_t invctr[]  = {0x07};
    const uint8_t pwctr1[]  = {0xA2,0x02,0x84};
    const uint8_t pwctr2[]  = {0xC5};
    const uint8_t pwctr3[]  = {0x0A,0x00};
    const uint8_t pwctr4[]  = {0x8A,0x2A};
    const uint8_t pwctr5[]  = {0x8A,0xEE};
    const uint8_t vmctr1[]  = {0x0E};
    const uint8_t madctl_d[] = {MADCTL_MX | MADCTL_MY | MADCTL_RGB};
    const uint8_t madctl_r[] = {MADCTL_MX | MADCTL_MV | MADCTL_RGB};
    const uint8_t colmod[] = {0x05};
    const uint8_t caset[] = {0x00, 0x00, 0x00, LCD_NATIVE_H_RES - 1};
    const uint8_t raset[] = {0x00, 0x00, 0x00, LCD_NATIVE_V_RES - 1};
    const uint8_t gp[] = {0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,
                          0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10};
    const uint8_t gn[] = {0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,
                          0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10};
    
    st7735_tx(io, ST7735_DISPOFF, NULL, 0);
    st7735_tx(io, ST7735_SWRESET, NULL, 0);
    st7735_delay(150);
    st7735_tx(io, ST7735_SLPOUT, NULL, 0);
    st7735_delay(500);
    st7735_tx(io, ST7735_FRMCTR1, frmctr, sizeof(frmctr));
    st7735_tx(io, ST7735_FRMCTR2, frmctr, sizeof(frmctr));
    st7735_tx(io, ST7735_FRMCTR3, frmctr3, sizeof(frmctr3));
    st7735_tx(io, ST7735_INVCTR, invctr, sizeof(invctr));
    st7735_tx(io, ST7735_PWCTR1, pwctr1, sizeof(pwctr1));
    st7735_tx(io, ST7735_PWCTR2, pwctr2, sizeof(pwctr2));
    st7735_tx(io, ST7735_PWCTR3, pwctr3, sizeof(pwctr3));
    st7735_tx(io, ST7735_PWCTR4, pwctr4, sizeof(pwctr4));
    st7735_tx(io, ST7735_PWCTR5, pwctr5, sizeof(pwctr5));
    st7735_tx(io, ST7735_VMCTR1, vmctr1, sizeof(vmctr1));
    st7735_tx(io, ST7735_INVOFF, NULL, 0);
    st7735_tx(io, ST7735_MADCTL, madctl_d, sizeof(madctl_d));
    st7735_tx(io, ST7735_COLMOD, colmod, sizeof(colmod));
    st7735_tx(io, ST7735_CASET, caset, sizeof(caset));
    st7735_tx(io, ST7735_RASET, raset, sizeof(raset));
    st7735_tx(io, ST7735_GMCTRP1, gp, sizeof(gp));
    st7735_tx(io, ST7735_GMCTRN1, gn, sizeof(gn));
    st7735_tx(io, ST7735_NORON, NULL, 0);
    st7735_delay(10);
    st7735_tx(io, ST7735_MADCTL, madctl_r, sizeof(madctl_r));
    st7735_tx(io, ST7735_DISPON, NULL, 0);
    st7735_delay(20);
}

static void lcd_init(void)
{
    ESP_LOGI(TAG, "Initializing LCD");
    
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = PIN_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .spi_mode = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &s_lcd_io));
    
    st7735_init(s_lcd_io);
    ESP_LOGI(TAG, "LCD initialized successfully");
}

/* ── SD Card ───────────────────────────────────────────────────────────── */
static void sdcard_init(void)
{
    ESP_LOGI(TAG, "Initializing SD card");
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = LCD_HOST;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_SD_CS;
    slot_config.host_id = host.slot;
    
    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &s_sd_card);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "SD card mounted successfully");
}

/* ── I2C ───────────────────────────────────────────────────────────────── */
static void i2c_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C");
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
    
    ESP_LOGI(TAG, "I2C initialized successfully");
}

/* ── Buzzer ────────────────────────────────────────────────────────────── */
static void buzzer_init(void)
{
    ESP_LOGI(TAG, "Initializing Buzzer");
    
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    
    ESP_LOGI(TAG, "Buzzer initialized successfully");
}

/* ── Combo Key Callbacks ──────────────────────────────────────────────── */
static void on_combo_up_b(void)
{
    ESP_LOGI(TAG, "Combo key: UP+B -> Home");
    ui_show_home();
}

static void on_combo_down_b(void)
{
    ESP_LOGI(TAG, "Combo key: DOWN+B -> Task Manager");
    ui_show_task_manager();
}

/* ── Key Event Handler ────────────────────────────────────────────────── */
static void handle_key_event(lv_key_t key)
{
    power_manager_reset_sleep_timer();
    
    if (key == LV_KEY_ESC) {
        ui_go_back();
        return;
    }
    
    ui_handle_key(key);
}

/* ── Main Application ─────────────────────────────────────────────────── */
void app_main(void)
{
    // 设置返回 loader（必须在最开头）
    return_to_loader_setup();
    
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "XiaoMiao OS v1.0.0 Starting...");
    ESP_LOGI(TAG, "=================================");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize hardware
    lcd_init();
    sdcard_init();
    i2c_init();
    buzzer_init();
    
    // Initialize power manager
    power_manager_init();
    power_manager_start_monitor();
    
    // Initialize LVGL
    lv_init();
    
    // Initialize LVGL port
    lvgl_port_init(s_lcd_io, NULL);
    
    // Initialize config manager
    config_manager_init();
    
    // Initialize application manager
    app_manager_init();
    app_manager_scan_apps();
    
    // Initialize MicroPython runtime
    micropython_runtime_init();
    
    // Initialize UI system
    ui_main_init();
    
    // Register combo keys
    lvgl_port_register_combo_key("up+b", on_combo_up_b);
    lvgl_port_register_combo_key("down+b", on_combo_down_b);
    
    // Register key event handler
    lvgl_port_register_key_event(handle_key_event);
    
    // Start button scan task
    xTaskCreate(lvgl_port_button_scan_task, "button_scan", 2048, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "XiaoMiao OS Ready!");
    ESP_LOGI(TAG, "=================================");
    
    // Main loop
    while (1) {
        lvgl_port_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
