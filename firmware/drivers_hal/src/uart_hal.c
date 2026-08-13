/**
 * @file uart_hal.c
 * @author Gabriel
 * @brief HAL driver for UART
 * @version 0.1
 * @date 2026-08-13
 *
 * @copyright Copyright (c) 2026
 *
 */

/** @defgroup hal HAL
 *  @brief Capa de abstraccion de hardware.
 *  @{
 */

/*==================[inclusions]=============================================*/
#include "uart_hal.h"
/* TODO: Include the specific ESP-IDF driver header for UART here */
#include "driver/uart.h"
#include "esp_err.h"
#include <string.h>
/*==================[macros and definitions]=================================*/
#ifndef UART_EVENT_QUEUE_SIZE
#define UART_EVENT_QUEUE_SIZE 10
#endif
/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/
static uart_hal_cfg_t s_cfg;
static bool s_initialized = false;
static uart_rx_cb_t s_rx_cb = NULL;
static void *s_rx_cb_arg = NULL;
static QueueHandle_t s_uart_queue = NULL;
/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/
static void uart_event_task(void *arg)
{
    uart_event_t event;
    for (;;) {
        if (xQueueReceive(s_uart_queue, &event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                size_t len = event.size;
                uint8_t *buf = (uint8_t *)pvPortMalloc(len);
                if (buf) {
                    int r = uart_read_bytes(s_cfg.uart_num, buf, (int)len, 0);
                    if (r > 0 && s_rx_cb) {
                        s_rx_cb(buf, (size_t)r, s_rx_cb_arg);
                    }
                    vPortFree(buf);
                } else {
                    uart_flush_input(s_cfg.uart_num);
                }
            }
            /* other event types can be handled here if needed */
        }
    }
}

/*==================[external functions definition]==========================*/
uint8_t UartHalInit(const uart_hal_cfg_t *cfg)
{
    if (!cfg) return false;
    if (s_initialized) UartHalDeinit();

    memcpy(&s_cfg, cfg, sizeof(s_cfg));

    uart_config_t uart_config = {
        .baud_rate = (int)s_cfg.baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    if (uart_param_config(s_cfg.uart_num, &uart_config) != ESP_OK) return false;
    if (uart_set_pin(s_cfg.uart_num, s_cfg.tx_pin, s_cfg.rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) return false;

    if (uart_driver_install(s_cfg.uart_num, s_cfg.rx_buffer_size, s_cfg.tx_buffer_size, UART_EVENT_QUEUE_SIZE, &s_uart_queue, 0) != ESP_OK) {
        return false;
    }

    /* create event task to process UART events */
    xTaskCreate(uart_event_task, "uart_evt", 2048, NULL, configMAX_PRIORITIES - 5, NULL);

    s_initialized = true;
    return true;
}

int UartHalWrite(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (!s_initialized || !data || len == 0) return 0;
    int written = uart_write_bytes(s_cfg.uart_num, (const char *)data, (int)len);
    /* optionally wait for TX done: uart_wait_tx_done(s_cfg.uart_num, pdMS_TO_TICKS(timeout_ms)); */
    return written;
}

int UartHalRead(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    if (!s_initialized || !buf || len == 0) return 0;
    int r = uart_read_bytes(s_cfg.uart_num, buf, (int)len, pdMS_TO_TICKS(timeout_ms));
    return r;
}

uint8_t UartHalSetBaud(uint32_t baudrate)
{
    if (!s_initialized) return false;
    uart_set_baudrate(s_cfg.uart_num, (int)baudrate);
    s_cfg.baudrate = baudrate;
    return true;
}

uint8_t UartHalInstallRxCb(uart_rx_cb_t cb, void *arg)
{
    s_rx_cb = cb;
    s_rx_cb_arg = arg;
    return true;
}

void UartHalDeinit(void)
{
    if (!s_initialized) return;
    uart_driver_delete(s_cfg.uart_num);
    s_initialized = false;
    s_rx_cb = NULL;
    s_rx_cb_arg = NULL;
    /* Note: event task and queue cleanup omitted for brevity */
}

/** @} */ /* end of defgroup hal */

/*==================[end of file]============================================*/
