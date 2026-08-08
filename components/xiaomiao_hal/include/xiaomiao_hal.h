#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Hardware Constants ──────────────────────────────────────────── */

#define XOS_LCD_HOST          SPI2_HOST
#define XOS_LCD_PIXEL_CLK_HZ  (60 * 1000 * 1000)
#define XOS_LCD_H_RES         160
#define XOS_LCD_V_RES         128
#define XOS_LCD_DRAW_BUF_LINES XOS_LCD_V_RES

#define XOS_PIN_LCD_SCLK   18
#define XOS_PIN_LCD_MOSI   23
#define XOS_PIN_LCD_MISO   19
#define XOS_PIN_LCD_CS     5
#define XOS_PIN_LCD_DC     4

#define XOS_PIN_SD_CS      22

#define XOS_PIN_I2C_SCL    15
#define XOS_PIN_I2C_SDA    21
#define XOS_I2C_FREQ_HZ    100000
#define XOS_I2C_TIMEOUT_MS 30

#define XOS_I2C_ADDR_GD32  0x40
#define XOS_I2C_ADDR_MPU6050 0x68

#define XOS_PIN_BUZZER     14

#define XOS_BTN_UP      0
#define XOS_BTN_DOWN    1
#define XOS_BTN_LEFT    2
#define XOS_BTN_RIGHT   3
#define XOS_BTN_A       4
#define XOS_BTN_B       5
#define XOS_NUM_BUTTONS  6

#define XOS_BTN_ACTIVE_LEVEL  0
#define XOS_BTN_DEBOUNCE_MS   25

/* Battery ADC — GPIO32 (original button A pin, now ADC1_CH4)
 * Divider: 9.1k (top) / 2.4k (bottom)
 * Vbat = Vadc * (9.1+2.4)/2.4 = Vadc * 4.792
 * Full-scale: 3.3V * 4.792 = 15.81V (safe for LiPo 4.2V max)
 */
#define XOS_BATT_ADC_CHAN    4    /* ADC1_CH4 = GPIO32 */
#define XOS_BATT_DIVIDER     4.792f
#define XOS_BATT_VMIN        3.3f
#define XOS_BATT_VMAX        4.2f

/* UI Colors */
#define XOS_CLR_YELLOW  0xF6D34A
#define XOS_CLR_BLACK   0x1B1713
#define XOS_CLR_BROWN   0x5C4220
#define XOS_CLR_RED     0xE64B3C
#define XOS_CLR_CREAM   0xFFF3B0
#define XOS_CLR_GREEN   0x2DD466

/* LVGL */
#define XOS_LVGL_TICK_MS    1
#define XOS_LVGL_TASK_STACK (10 * 1024)
#define XOS_LVGL_TASK_PRIO  5

/* SD card mount point */
#define XOS_SD_MOUNT_POINT  "/sdcard"
#define XOS_APPS_DIR        "/sdcard/apps"
#define XOS_DATA_DIR        "/sdcard/data"
#define XOS_SYSTEM_DIR      "/sdcard/system"
#define XOS_ROMS_DIR        "/sdcard/roms"

/* ── HAL API ────────────────────────────────────────────────────── */

typedef struct {
    bool lcd_ok;
    bool sd_ok;
    bool i2c_ok;
    bool buzzer_ok;
    bool buttons_ok;
    bool battery_ok;
    float battery_voltage;
    uint8_t battery_percent;
} xos_hal_status_t;

/* Initialize all hardware subsystems */
esp_err_t xos_hal_init(xos_hal_status_t *status);

/* LCD */
esp_err_t xos_lcd_init(void);
lv_display_t *xos_display_init(void);

/* Buttons */
esp_err_t xos_buttons_init(void);
void xos_keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data);

/* SD card */
esp_err_t xos_sd_init(void);

/* I2C */
esp_err_t xos_i2c_init(void);

/* Buzzer */
esp_err_t xos_buzzer_init(void);
void xos_buzzer_set_freq(uint32_t freq_hz);
void xos_buzzer_off(void);

/* Battery */
esp_err_t xos_battery_init(void);
float xos_battery_read_voltage(void);
uint8_t xos_battery_read_percent(void);

/* GD32 I2C device */
esp_err_t xos_gd32_set_led(uint8_t led_num, bool on);
esp_err_t xos_gd32_set_motor(uint8_t motor_num, uint8_t speed, bool forward);

/* MPU6050 */
esp_err_t xos_mpu6050_init(void);
esp_err_t xos_mpu6050_read_accel(int16_t *x, int16_t *y, int16_t *z);
esp_err_t xos_mpu6050_read_gyro(int16_t *x, int16_t *y, int16_t *z);

/* Hardware self-test (for phase 1 verification) */
esp_err_t xos_hal_self_test(xos_hal_status_t *status);

#ifdef __cplusplus
}
#endif