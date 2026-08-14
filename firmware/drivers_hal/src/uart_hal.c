#include "uart_hal.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <string.h>

static const char *TAG = "uart_hal";

bool UartHalInit(const uart_hal_cfg_t *cfg)
{
    if (!cfg) {
        ESP_LOGE(TAG, "cfg NULL");
        return false;
    }
    if (cfg->uart_num >= UART_NUM_MAX) {
        ESP_LOGE(TAG, "invalid uart num %d", cfg->uart_num);
        return false;
    }

    uart_config_t ucfg = {
        .baud_rate = cfg->baudrate,
        .data_bits = cfg->data_bits,
        .parity = cfg->parity,
        .stop_bits = cfg->stop_bits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    /* Fijado para ESP32-C6 */
    ucfg.source_clk = UART_SCLK_RTC;

    esp_err_t err = uart_param_config(cfg->uart_num, &ucfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %d", err);
        return false;
    }

    err = uart_set_pin(cfg->uart_num, cfg->tx_pin, cfg->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %d", err);
        return false;
    }

    err = uart_driver_install(cfg->uart_num, (int)cfg->rx_buffer_size, (int)cfg->tx_buffer_size, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %d", err);
        return false;
    }

    ESP_LOGI(TAG, "UART%u init tx=%d rx=%d baud=%d", cfg->uart_num, cfg->tx_pin, cfg->rx_pin, cfg->baudrate);
    return true;
}

int UartHalWrite(uart_port_t uart_num, const void *data, size_t len, TickType_t ticks_to_wait)
{
    if (uart_num >= UART_NUM_MAX || !data || len == 0) return -1;
    int written = uart_write_bytes(uart_num, (const char *)data, len);
    return written;
}

int UartHalRead(uart_port_t uart_num, void *buf, size_t len, TickType_t ticks_to_wait)
{
    if (uart_num >= UART_NUM_MAX || !buf || len == 0) return -1;

    uint32_t timeout_ms;
    if (ticks_to_wait == portMAX_DELAY) {
        timeout_ms = portMAX_DELAY;
    } else {
        timeout_ms = (uint32_t)(ticks_to_wait * portTICK_PERIOD_MS);
        if (timeout_ms == 0) timeout_ms = 1;
    }

    int r = uart_read_bytes(uart_num, buf, len, timeout_ms);
    return r;
}

void UartHalFlush(uart_port_t uart_num)
{
    if (uart_num >= UART_NUM_MAX) return;
    uart_flush(uart_num);
}

void UartHalDeinit(uart_port_t uart_num)
{
    if (uart_num >= UART_NUM_MAX) return;
    uart_driver_delete(uart_num);
    ESP_LOGI(TAG, "UART%u deinitialized", uart_num);
}
