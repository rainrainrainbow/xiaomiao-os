#pragma once
#include "esp_err.h"

#define MPU6050_ADDR  0x68
#define MPU6050_PWR_MGMT_1  0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

typedef struct {
    int16_t acc_x, acc_y, acc_z;
    int16_t gyro_x, gyro_y, gyro_z;
    float    temp_c;
} imu_data_t;

esp_err_t gyro_init(void);
esp_err_t gyro_read(imu_data_t *data);
int16_t    gyro_acc_x(void);
int16_t    gyro_acc_y(void);
int16_t    gyro_acc_z(void);
int16_t    gyro_gyro_z(void);
float      gyro_temperature(void);
