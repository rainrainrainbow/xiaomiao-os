/**
 * @file main.c
 * @brief XiaoMiao OS - MicroPython Desktop System
 * 
 * Integrated main entry point combining:
 * - Hardware initialization (LCD, SD, buttons, I2C, buzzer, battery)
 * - LVGL display and input drivers
 * - Application manager (SD card scanning)
 * - MicroPython runtime
 * - Desktop UI system
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

#include "lvgl_port.h"
#include "app_manager.h"
#include "micropython_runtime.h"
#include "config_manager.h"
#include "power_manager.h"
#include "ui_main.h"

static const char *TAG = "main";

/* ── Hardware Configuration ─────────────────────────────────────────── */

// LCD (ST7735 160x128)
#define LCD_HOST            SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ  (40 * 1000 * 1000)
#define PIN_LCD_SCLK        18
#define PIN_LCD_MOSI        23
#define PIN_LCD_MISO        19
#define PIN_LCD_CS          5
#define PIN_LCD_DC          4
#define PIN_LCD_RST         -1
#define LCD_H_RES           160
#define LCD_V_RES           128

// SD Card
#define PIN_SD_CS           22

// I2C
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SCL_IO   15
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_FREQ_HZ  100000

// Buzzer
#define BUZZER_GPIO         GPIO_NUM_14
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_CHANNEL        LEDC_CHANNEL_0

// Battery ADC is managed by power_manager module

/* ── Global Handles ─────────────────────────────────────────────────── */

static esp_lcd_panel_io_handle_t s_lcd_io = NULL;
static esp_lcd_panel_handle_t s_lcd_panel = NULL;
static sdmmc_card_t *s_sd_card = NULL;

/* ── LCD Initialization ─────────────────────────────────────────────── */

/* ── ST7735 Register Definitions ───────────────────────────────────── */

#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_NORON   0x13
#define ST7735_INVOFF  0x20
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08

static void st7735_write_cmd(esp_lcd_panel_io_handle_t io, uint8_t cmd)
{
    esp_lcd_panel_io_tx_param(io, cmd, NULL, 0);
}

static void st7735_write_data(esp_lcd_panel_io_handle_t io, const void *data, size_t len)
{
    esp_lcd_panel_io_tx_param(io, 0x00, data, len);
}

static void st7735_init(esp_lcd_panel_io_handle_t io)
{
    // Software reset
    st7735_write_cmd(io, ST7735_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    
    // Sleep out
    st7735_write_cmd(io, ST7735_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Frame rate control
    uint8_t frmctr1[] = {0x01, 0x2C, 0x2D};
    st7735_write_data(io, frmctr1, sizeof(frmctr1));
    
    uint8_t frmctr2[] = {0x01, 0x2C, 0x2D};
    st7735_write_data(io, frmctr2, sizeof(frmctr2));
    
    uint8_t frmctr3[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
    st7735_write_data(io, frmctr3, sizeof(frmctr3));
    
    // Display inversion control
    uint8_t invctr[] = {0x07};
    st7735_write_data(io, invctr, sizeof(invctr));
    
    // Power control
    uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
    st7735_write_data(io, pwctr1, sizeof(pwctr1));
    
    uint8_t pwctr2[] = {0xC5};
    st7735_write_data(io, pwctr2, sizeof(pwctr2));
    
    uint8_t pwctr3[] = {0x0A, 0x00};
    st7735_write_data(io, pwctr3, sizeof(pwctr3));
    
    uint8_t pwctr4[] = {0x8A, 0x2A};
    st7735_write_data(io, pwctr4, sizeof(pwctr4));
    
    uint8_t pwctr5[] = {0x8A, 0xEE};
    st7735_write_data(io, pwctr5, sizeof(pwctr5));
    
    // VCOM control
    uint8_t vmctr1[] = {0x0E};
    st7735_write_data(io, vmctr1, sizeof(vmctr1));
    
    // Display inversion off
    st7735_write_cmd(io, ST7735_INVOFF);
    
    // Memory access control (rotation)
    uint8_t madctl = MADCTL_MV | MADCTL_MY | MADCTL_RGB;
    st7735_write_data(io, &madctl, 1);
    
    // Color mode (16-bit)
    uint8_t colmod = 0x05;
    st7735_write_data(io, &colmod, 1);
    
    // Normal display mode on
    st7735_write_cmd(io, ST7735_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Display on
    st7735_write_cmd(io, ST7735_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void lcd_init(void)
{
    ESP_LOGI(TAG, "Initializing LCD (ST7735 160x128)");
    
    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = PIN_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    // LCD panel IO
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
    
    // Initialize ST7735
    st7735_init(s_lcd_io);
    
    ESP_LOGI(TAG, "LCD initialized successfully");
}

/* ── SD Card Initialization ─────────────────────────────────────────── */

static void sdcard_init(void)
{
    ESP_LOGI(TAG, "Initializing SD Card");
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    // SD card shares SPI2 bus with LCD (MOSI=23, MISO=19, SCLK=18)
    // Only needs its own CS pin (GPIO22)
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = LCD_HOST;  // Share SPI2 with LCD
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

/* ── I2C Initialization ─────────────────────────────────────────────── */

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

/* ── Buzzer Initialization ──────────────────────────────────────────── */

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

/* ── Battery ADC (now handled by power_manager) ─────────────────────── */
// Battery ADC initialization moved to power_manager module
// to avoid resource conflicts.

/* ── Combo Key Callbacks ────────────────────────────────────────────── */

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

/* ── Key Event Handler ──────────────────────────────────────────────── */

// 按键事件处理（从 lvgl_port 分发到 UI）
static void handle_key_event(lv_key_t key)
{
    // 重置休眠计时器（用户活动）
    power_manager_reset_sleep_timer();
    
    // B 键 = 返回
    if (key == LV_KEY_ESC) {
        ui_go_back();
        return;
    }
    
    // 其他按键传递给 UI 处理
    ui_handle_key(key);
}

/* ── Main Application ───────────────────────────────────────────────── */

void app_main(void)
{
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
    
    // Initialize power manager (includes battery ADC monitoring)
    power_manager_init();
    power_manager_start_monitor();
    
    // Initialize LVGL
    lv_init();
    
    // Initialize LVGL port (display and input)
    lvgl_port_init(s_lcd_io, s_lcd_panel);
    
    // Initialize config manager (load settings from SD card)
    config_manager_init();
    
    // Initialize application manager
    app_manager_init();
    app_manager_scan_apps();
    
    // Initialize MicroPython runtime
    micropython_runtime_init();
    
    // Initialize UI system (will load theme from config)
    ui_main_init();
    
    // Register combo keys
    lvgl_port_register_combo_key("up+b", on_combo_up_b);
    lvgl_port_register_combo_key("down+b", on_combo_down_b);
    
    // Register single key event handler
    lvgl_port_register_key_event(handle_key_event);
    
    // Start button scan task
    xTaskCreate(lvgl_port_button_scan_task, "button_scan", 2048, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "XiaoMiao OS Ready!");
    ESP_LOGI(TAG, "=================================");
    
    // Main loop - LVGL task handler
    while (1) {
        lvgl_port_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
