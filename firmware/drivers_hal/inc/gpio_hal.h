/* drivers_hal/inc/uart_hal.h */
#ifndef UART_HAL_H_
#define UART_HAL_H_

#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int uart_hal_port_t; /* 0,1,... */

typedef struct {
    uart_hal_port_t port;    /* e.g., 1 for UART1 */
    int tx_pin;              /* GPIO number for TX (e.g., 18) */
    int rx_pin;              /* GPIO number for RX (e.g., 19) */
    int baudrate;            /* e.g., 115200 */
    size_t rx_buffer_size;   /* bytes for driver RX buffer */
    size_t tx_buffer_size;   /* bytes for driver TX buffer */
    bool use_flow_ctrl;      /* false if RTS/CTS not wired */
} uart_hal_cfg_t;

/* Inicializa el UART según cfg. Retorna true si OK */
bool UartHalInit(const uart_hal_cfg_t *cfg);

/* Escribe len bytes; espera ticks_to_wait. Retorna bytes escritos o -1 */
int UartHalWrite(uart_hal_port_t port, const void *data, size_t len, TickType_t ticks_to_wait);

/* Lee hasta len bytes; espera ticks_to_wait. Retorna bytes leídos o -1 */
int UartHalRead(uart_hal_port_t port, void *buf, size_t len, TickType_t ticks_to_wait);

/* Vacía buffers */
void UartHalFlush(uart_hal_port_t port);

/* Desinicializa el puerto */
void UartHalDeinit(uart_hal_port_t port);

#ifdef __cplusplus
}
#endif

#endif /* UART_HAL_H_ */
