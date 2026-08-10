// ================ mpu6050.h - I2C IMU 驱动 ================
// SCL=15  SDA=21  I2C 100kHz  Addr 0x68

#ifndef __MPU6050_H__
#define __MPU6050_H__

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ax, ay, az;   // g
    float gx, gy, gz;   // deg/s
    float temperature;  // ℃
} mpu6050_data_t;

esp_err_t mpu6050_init(void);
bool mpu6050_is_ready(void);
esp_err_t mpu6050_read(mpu6050_data_t *out);
esp_err_t mpu6050_sleep(void);
esp_err_t mpu6050_wake(void);

#ifdef __cplusplus
}
#endif

#endif // __MPU6050_H__