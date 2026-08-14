#ifndef UART_HAL_H_
#define UART_HAL_H_

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uart_port_t uart_num;
    int tx_pin;
    int rx_pin;
    int rts_pin;
    int cts_pin;
    int baudrate;
    size_t rx_buffer_size;
    size_t tx_buffer_size;
    bool use_flow_ctrl;
    uart_word_length_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    int event_queue_size;
} uart_hal_cfg_t;

bool UartHalInit(const uart_hal_cfg_t *cfg);
int UartHalWrite(uart_port_t uart_num, const void *data, size_t len, TickType_t ticks_to_wait);
int UartHalRead(uart_port_t uart_num, void *buf, size_t len, TickType_t ticks_to_wait);
void UartHalFlush(uart_port_t uart_num);
void UartHalDeinit(uart_port_t uart_num);

#ifdef __cplusplus
}
#endif

#endif /* UART_HAL_H_ */
