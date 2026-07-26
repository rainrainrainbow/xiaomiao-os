/*
 * gyro.c - MPU6050 六轴传感器驱动 (I2C)
 *
 * 默认 I2C 地址 0x68
 * 加速度量程 ±2g, 陀螺仪 ±250°/s
 */
#include "esp_log.h"
#include "hal/i2c_bus.h"
#include "hal/gyro.h"

static const char *TAG = "gyro";
static bool s_initialized = false;

esp_err_t gyro_init(void)
{
    ESP_LOGI(TAG, "Initializing MPU6050 (I2C 0x%02X)...", MPU6050_ADDR);

    /* 唤醒 MPU6050: PWR_MGMT_1 = 0 */
    esp_err_t ret = i2c_bus_write_reg(MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 wakeup failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 设置加速度量程 ±2g: ACCEL_CONFIG = 0 */
    i2c_bus_write_reg(MPU6050_ADDR, 0x1C, 0x00);

    /* 设置陀螺仪量程 ±250°/s: GYRO_CONFIG = 0 */
    i2c_bus_write_reg(MPU6050_ADDR, 0x1B, 0x00);

    s_initialized = true;
    ESP_LOGI(TAG, "MPU6050 ready");
    return ESP_OK;
}

esp_err_t gyro_read(imu_data_t *data)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    uint8_t buf[14];
    esp_err_t ret = i2c_bus_read_regs(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, buf, 14);
    if (ret != ESP_OK) return ret;

    data->acc_x  = (int16_t)((buf[0] << 8) | buf[1]);
    data->acc_y  = (int16_t)((buf[2] << 8) | buf[3]);
    data->acc_z  = (int16_t)((buf[4] << 8) | buf[5]);
    data->gyro_x = (int16_t)((buf[8] << 8) | buf[9]);
    data->gyro_y = (int16_t)((buf[10] << 8) | buf[11]);
    data->gyro_z = (int16_t)((buf[12] << 8) | buf[13]);
    data->temp_c = ((int16_t)((buf[6] << 8) | buf[7])) / 340.0f + 36.53f;

    return ESP_OK;
}

int16_t gyro_acc_x(void)       { imu_data_t d; gyro_read(&d); return d.acc_x; }
int16_t gyro_acc_y(void)       { imu_data_t d; gyro_read(&d); return d.acc_y; }
int16_t gyro_acc_z(void)       { imu_data_t d; gyro_read(&d); return d.acc_z; }
int16_t gyro_gyro_z(void)      { imu_data_t d; gyro_read(&d); return d.gyro_z; }
float   gyro_temperature(void)  { imu_data_t d; gyro_read(&d); return d.temp_c; }
