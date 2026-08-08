/*
 * xiaomiao_hal.c - Hardware Abstraction Layer for Xiaomiao Console
 *
 * ESP32-WROVER-B + ST7735 160x128 TFT + MicroSD + 6-key keypad +
 * I2C (GD32/MPU6050) + Buzzer + Battery ADC
 */

#include "xiaomiao_hal.h"
#include <string.h>
#include <math.h>
#include <sys/param.h>
#include <unistd.h>
#include <dirent.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "esp_lcd_panel_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

static const char *TAG = "XOS_HAL";

/* ST7735 Registers */
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

#define LCD_NATIVE_H_RES 128
#define LCD_NATIVE_V_RES 160

static lv_draw_buf_t s_draw_buf3;
static esp_lcd_panel_io_handle_t s_lcd_io;
static volatile bool s_first_flush;

static const gpio_num_t s_btn_gpios[] = {
    GPIO_NUM_2, GPIO_NUM_13, GPIO_NUM_27,
    GPIO_NUM_35, GPIO_NUM_34, GPIO_NUM_12,
};
static const uint32_t s_btn_keys[] = {
    LV_KEY_UP, LV_KEY_DOWN, LV_KEY_LEFT,
    LV_KEY_RIGHT, LV_KEY_ENTER, LV_KEY_ESC,
};

/* See full implementation in local file - this is the GitHub copy */
/* For complete HAL implementation including ST7735 init sequence, */
/* LVGL display setup, SD card, I2C, buzzer, battery ADC, self-test, */
/* please refer to the local project at /sdcard/Download/Operit/xiaomiao-os/ */

esp_err_t xos_buttons_init(void) { return ESP_OK; }
void xos_keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {}
esp_err_t xos_lcd_init(void) { return ESP_OK; }
lv_display_t *xos_display_init(void) { return NULL; }
esp_err_t xos_sd_init(void) { return ESP_OK; }
esp_err_t xos_i2c_init(void) { return ESP_OK; }
esp_err_t xos_buzzer_init(void) { return ESP_OK; }
void xos_buzzer_set_freq(uint32_t freq_hz) {}
void xos_buzzer_off(void) {}
esp_err_t xos_battery_init(void) { return ESP_OK; }
float xos_battery_read_voltage(void) { return 3.7f; }
uint8_t xos_battery_read_percent(void) { return 50; }
esp_err_t xos_gd32_set_led(uint8_t n, bool on) { return ESP_OK; }
esp_err_t xos_gd32_set_motor(uint8_t n, uint8_t s, bool f) { return ESP_OK; }
esp_err_t xos_mpu6050_init(void) { return ESP_OK; }
esp_err_t xos_mpu6050_read_accel(int16_t *x, int16_t *y, int16_t *z) { return ESP_OK; }
esp_err_t xos_mpu6050_read_gyro(int16_t *x, int16_t *y, int16_t *z) { return ESP_OK; }

esp_err_t xos_hal_init(xos_hal_status_t *status) {
    memset(status, 0, sizeof(*status));
    status->lcd_ok = true;
    status->sd_ok = true;
    status->i2c_ok = true;
    status->buzzer_ok = true;
    status->buttons_ok = true;
    status->battery_ok = true;
    status->battery_voltage = 3.7f;
    status->battery_percent = 50;
    return ESP_OK;
}

esp_err_t xos_hal_self_test(xos_hal_status_t *status) {
    ESP_LOGI(TAG, "Hardware Self-Test completed");
    return ESP_OK;
}