/* drivers_hal/src/uart_hal.c */
#include "uart_hal.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "uart_hal";

/* Defaults helper */
static inline int _default_rx_buf(size_t s) { return (int)(s ? s : 1024); }
static inline int _default_tx_buf(size_t s) { return (int)(s ? s : 1024); }

bool UartHalInit(const uart_hal_cfg_t *cfg)
{
    if (!cfg) {
        ESP_LOGE(TAG, "cfg NULL");
        return false;
    }
    if (cfg->port < 0 || cfg->port >= UART_NUM_MAX) {
        ESP_LOGE(TAG, "invalid uart port %d", cfg->port);
        return false;
    }

    uart_config_t ucfg = {
        .baud_rate = cfg->baudrate ? cfg->baudrate : 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = cfg->use_flow_ctrl ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    /* ESP32-C6 specific clock selection */
    ucfg.source_clk = UART_SCLK_RTC;

    esp_err_t err = uart_param_config((uart_port_t)cfg->port, &ucfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %d", err);
        return false;
    }

    err = uart_set_pin((uart_port_t)cfg->port,
                       cfg->tx_pin,
                       cfg->rx_pin,
                       (cfg->use_flow_ctrl ? UART_PIN_NO_CHANGE : UART_PIN_NO_CHANGE),
                       (cfg->use_flow_ctrl ? UART_PIN_NO_CHANGE : UART_PIN_NO_CHANGE));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %d", err);
        return false;
    }

    int rx_buf = _default_rx_buf(cfg->rx_buffer_size);
    int tx_buf = _default_tx_buf(cfg->tx_buffer_size);

    err = uart_driver_install((uart_port_t)cfg->port, rx_buf, tx_buf, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %d", err);
        return false;
    }

    ESP_LOGI(TAG, "UART%u initialized (tx=%d rx=%d baud=%d)", cfg->port, cfg->tx_pin, cfg->rx_pin, ucfg.baud_rate);
    return true;
}

int UartHalWrite(uart_hal_port_t port, const void *data, size_t len, TickType_t ticks_to_wait)
{
    if (port < 0 || port >= UART_NUM_MAX || !data || len == 0) return -1;
    int written = uart_write_bytes((uart_port_t)port, (const char *)data, len);
    return written;
}

int UartHalRead(uart_hal_port_t port, void *buf, size_t len, TickType_t ticks_to_wait)
{
    if (port < 0 || port >= UART_NUM_MAX || !buf || len == 0) return -1;

    uint32_t timeout_ms;
    if (ticks_to_wait == portMAX_DELAY) {
        timeout_ms = portMAX_DELAY;
    } else {
        timeout_ms = (uint32_t)(ticks_to_wait * portTICK_PERIOD_MS);
        if (timeout_ms == 0) timeout_ms = 1;
    }

    int r = uart_read_bytes((uart_port_t)port, buf, len, timeout_ms);
    return r;
}

void UartHalFlush(uart_hal_port_t port)
{
    if (port < 0 || port >= UART_NUM_MAX) return;
    uart_flush((uart_port_t)port);
}

void UartHalDeinit(uart_hal_port_t port)
{
    if (port < 0 || port >= UART_NUM_MAX) return;
    uart_driver_delete((uart_port_t)port);
    ESP_LOGI(TAG, "UART%u deinitialized", port);
}
