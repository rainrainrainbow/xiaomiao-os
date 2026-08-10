// ================ led_motor.c - GD32 通信 ================

#include "led_motor.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "gd32";

#define UART_NUM  UART_NUM_0
#define TXD_PIN   1
#define RXD_PIN   3
#define BAUDRATE  115200

#define STX 0xAA
#define CMD_LED   0x01
#define CMD_MOTOR 0x02

static volatile bool s_connected = false;
static uint8_t s_last_led = 0;
static uint8_t s_last_motor = 0;

// 探测任务
static void gd32_probe_task(void *arg)
{
    while (1) {
        uint8_t probe[] = {STX, 0x01, 0x00, 0x00};
        probe[3] = probe[2];  // checksum = cmd byte
        uart_write_bytes(UART_NUM, (const char *)probe, sizeof(probe));
        uint8_t resp[16];
        int n = uart_read_bytes(UART_NUM, resp, sizeof(resp), pdMS_TO_TICKS(100));
        if (n > 0 && resp[0] == STX) {
            s_connected = true;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void send_cmd(uint8_t cmd, uint8_t val)
{
    uint8_t pkt[5] = {STX, 0x02, cmd, val, 0};
    pkt[4] = cmd ^ val;  // XOR 校验
    uart_write_bytes(UART_NUM, (const char *)pkt, sizeof(pkt));
}

esp_err_t gd32_init(void)
{
    uart_config_t cfg = {
        .baud_rate = BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_RETURN_ON_ERROR(uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0),
                        TAG, "install");
    ESP_RETURN_ON_ERROR(uart_param_config(UART_NUM, &cfg), TAG, "cfg");
    ESP_RETURN_ON_ERROR(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, -1, -1), TAG, "pin");

    xTaskCreate(gd32_probe_task, "gd32_probe", 2048, NULL, 3, NULL);
    ESP_LOGI(TAG, "GD32 init OK (probe running)");
    return ESP_OK;
}

bool gd32_is_connected(void) { return s_connected; }

esp_err_t gd32_set_led(uint8_t mode)
{
    s_last_led = mode;
    send_cmd(CMD_LED, mode);
    return ESP_OK;
}

esp_err_t gd32_set_motor(uint8_t level)
{
    s_last_motor = level;
    send_cmd(CMD_MOTOR, level);
    return ESP_OK;
}