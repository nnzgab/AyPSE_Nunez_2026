#include "uart_hal.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *UART_TAG = "uart_hal";

#ifndef UART_HAL_NUM
#define UART_HAL_NUM UART_NUM_1
#endif

/* Pines por defecto (pueden ser redefinidos en board_config.h) */
#ifndef UART_HAL_TX_PIN
#define UART_HAL_TX_PIN 18
#endif
#ifndef UART_HAL_RX_PIN
#define UART_HAL_RX_PIN 19
#endif

#define UART_RX_BUF_SIZE 2048

static void uart_hal_configure_pins(int tx_pin, int rx_pin)
{
    /* Configuración básica de los pines similar al GPIO HAL */
    gpio_reset_pin((gpio_num_t)tx_pin);
    gpio_set_direction((gpio_num_t)tx_pin, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode((gpio_num_t)tx_pin, GPIO_PULLUP_ONLY);

    gpio_reset_pin((gpio_num_t)rx_pin);
    gpio_set_direction((gpio_num_t)rx_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)rx_pin, GPIO_PULLUP_ONLY);
}

void UartHalInitWithPins(int baud, int tx_pin, int rx_pin)
{
    const uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_LOGI(UART_TAG, "InitWithPins TX=%d RX=%d baud=%d", tx_pin, rx_pin, baud);
    uart_hal_configure_pins(tx_pin, rx_pin);

    esp_err_t err;
    err = uart_driver_install(UART_HAL_NUM, UART_RX_BUF_SIZE, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(UART_TAG, "uart_driver_install failed: %d", err);
        return;
    }
    err = uart_param_config(UART_HAL_NUM, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(UART_TAG, "uart_param_config failed: %d", err);
        return;
    }
    err = uart_set_pin(UART_HAL_NUM, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(UART_TAG, "uart_set_pin failed: %d", err);
        return;
    }
    ESP_LOGI(UART_TAG, "UART initialized");
}

void UartHalInit(int baud)
{
    /* Usa los pines por defecto (posibles overrides en board_config.h) */
    UartHalInitWithPins(baud, UART_HAL_TX_PIN, UART_HAL_RX_PIN);
}

int UartHalReadByte(char *rcv)
{
    if (rcv == NULL) return -1;
    int r = uart_read_bytes(UART_HAL_NUM, (uint8_t *)rcv, 1, pdMS_TO_TICKS(100));
    return r;
}

int UartHalReadBytes(char *buf, size_t len, uint32_t timeout_ms)
{
    if (buf == NULL || len == 0) return 0;
    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    int r = uart_read_bytes(UART_HAL_NUM, (uint8_t *)buf, len, ticks);
    return r;
}

void UartHalWriteByte(char tx)
{
    int w = uart_write_bytes(UART_HAL_NUM, &tx, 1);
    (void)w;
}

void UartHalWriteBytes(const char *buf, size_t len)
{
    if (buf == NULL || len == 0) return;
    int w = uart_write_bytes(UART_HAL_NUM, buf, len);
    ESP_LOGD(UART_TAG, "uart_write_bytes returned %d for len %d", w, (int)len);
    /* esperar a que termine la transmisión para evitar truncados en pruebas */
    uart_wait_tx_done(UART_HAL_NUM, pdMS_TO_TICKS(200));
}
