// ================ mpu6050.c - I2C 100kHz IMU ================

#include "mpu6050.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "mpu6050";

#define I2C_PORT       I2C_NUM_0
#define I2C_SCL_PIN    15
#define I2C_SDA_PIN    21
#define I2C_FREQ_HZ    100000
#define MPU_ADDR       0x68

#define REG_SMPLRT_DIV   0x19
#define REG_PWR_MGMT_1   0x6B
#define REG_ACCEL_XOUT_H 0x3B
#define REG_GYRO_XOUT_H  0x43
#define REG_CONFIG       0x1A
#define REG_GYRO_CONFIG  0x1B
#define REG_ACCEL_CONFIG 0x1C

static bool s_ready = false;

static esp_err_t i2c_init(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    return i2c_param_config(I2C_PORT, &cfg) == ESP_OK
        ? i2c_driver_install(I2C_PORT, cfg.mode, 0, 0, 0)
        : ESP_FAIL;
}

static esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_master_write_to_device(I2C_PORT, MPU_ADDR, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t read_regs(uint8_t reg, uint8_t *buf, int len)
{
    return i2c_master_write_read_device(I2C_PORT, MPU_ADDR, &reg, 1, buf, len, pdMS_TO_TICKS(100));
}

esp_err_t mpu6050_init(void)
{
    ESP_RETURN_ON_ERROR(i2c_init(), TAG, "i2c");

    // 唤醒 + 配置
    write_reg(REG_PWR_MGMT_1, 0x00);   // 唤醒
    vTaskDelay(pdMS_TO_TICKS(50));
    write_reg(REG_SMPLRT_DIV, 0x07);   // 1kHz / 8 = 125Hz
    write_reg(REG_CONFIG, 0x03);       // DLPF 44Hz
    write_reg(REG_GYRO_CONFIG, 0x10);  // ±1000°/s
    write_reg(REG_ACCEL_CONFIG, 0x10); // ±8g

    // 探测
    uint8_t who = 0;
    if (read_regs(0x75, &who, 1) != ESP_OK || who != 0x68) {
        ESP_LOGW(TAG, "MPU6050 not found (who=0x%02X)", who);
        s_ready = false;
        return ESP_ERR_NOT_FOUND;
    }

    s_ready = true;
    ESP_LOGI(TAG, "MPU6050 ready (0x68)");
    return ESP_OK;
}

bool mpu6050_is_ready(void) { return s_ready; }

esp_err_t mpu6050_read(mpu6050_data_t *out)
{
    if (!s_ready || !out) return ESP_ERR_INVALID_STATE;

    uint8_t buf[14];
    ESP_RETURN_ON_ERROR(read_regs(REG_ACCEL_XOUT_H, buf, 14), TAG, "burst");

    int16_t ax = (buf[0] << 8) | buf[1];
    int16_t ay = (buf[2] << 8) | buf[3];
    int16_t az = (buf[4] << 8) | buf[5];
    int16_t t  = (buf[6] << 8) | buf[7];
    int16_t gx = (buf[8] << 8) | buf[9];
    int16_t gy = (buf[10] << 8) | buf[11];
    int16_t gz = (buf[12] << 8) | buf[13];

    // ±8g → 4096 LSB/g, ±1000°/s → 32.8 LSB/(°/s)
    out->ax = ax / 4096.0f;
    out->ay = ay / 4096.0f;
    out->az = az / 4096.0f;
    out->gx = gx / 32.8f;
    out->gy = gy / 32.8f;
    out->gz = gz / 32.8f;
    out->temperature = t / 340.0f + 36.53f;
    return ESP_OK;
}

esp_err_t mpu6050_sleep(void)
{
    return write_reg(REG_PWR_MGMT_1, 0x40);
}

esp_err_t mpu6050_wake(void)
{
    return write_reg(REG_PWR_MGMT_1, 0x00);
}