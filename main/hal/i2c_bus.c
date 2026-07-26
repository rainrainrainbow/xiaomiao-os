/*
 * i2c_bus.c - I2C 总线驱动 (100kHz, SCL=15, SDA=21)
 *
 * 挂载设备:
 *   - GD32F350G8 协处理器 (地址 0x18)
 *   - MPU6050 陀螺仪 (地址 0x68)
 */
#include "esp_log.h"
#include "driver/i2c.h"
#include "hal/i2c_bus.h"

static const char *TAG = "i2c";

esp_err_t i2c_bus_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C (SCL=%d, SDA=%d, %dHz)...",
             I2C_SCL_PIN, I2C_SDA_PIN, I2C_FREQ_HZ);

    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT_NUM, &cfg));
    return i2c_driver_install(I2C_PORT_NUM, I2C_MODE_MASTER, 0, 0, 0);
}

esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, (uint8_t *)data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_bus_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_bus_write(addr, buf, 2);
}

esp_err_t i2c_bus_read_reg(uint8_t addr, uint8_t reg, uint8_t *val)
{
    esp_err_t ret = i2c_bus_write(addr, &reg, 1);
    if (ret != ESP_OK) return ret;
    return i2c_bus_read(addr, val, 1);
}

esp_err_t i2c_bus_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
    esp_err_t ret = i2c_bus_write(addr, &reg, 1);
    if (ret != ESP_OK) return ret;
    return i2c_bus_read(addr, buf, len);
}
