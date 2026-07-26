#pragma once
#include "esp_err.h"
#include "driver/i2c.h"

#define I2C_SCL_PIN  15
#define I2C_SDA_PIN  21
#define I2C_FREQ_HZ  100000  /* 100kHz */
#define I2C_PORT_NUM I2C_NUM_0

esp_err_t i2c_bus_init(void);
esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len);
esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len);
esp_err_t i2c_bus_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
esp_err_t i2c_bus_read_reg(uint8_t addr, uint8_t reg, uint8_t *val);
esp_err_t i2c_bus_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len);
