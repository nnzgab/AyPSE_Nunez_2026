#ifndef DRIVERS_HAL_UART_HAL_H_
#define DRIVERS_HAL_UART_HAL_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Inicializa UART con baudrate (usa pines por defecto o los definidos en board_config.h) */
void UartHalInit(int baud);

/** Inicializa UART con baudrate y pines explícitos (opcional) */
void UartHalInitWithPins(int baud, int tx_pin, int rx_pin);

/** Lee un byte. Devuelve >0 si leyó 1 byte, 0 si timeout, <0 en error */
int  UartHalReadByte(char *rcv);

/** Lee hasta len bytes o hasta timeout_ms. Devuelve cantidad leída */
int  UartHalReadBytes(char *buf, size_t len, uint32_t timeout_ms);

/** Escribe un byte (no bloqueante) */
void UartHalWriteByte(char tx);

/** Escribe len bytes; espera a que termine la transmisión */
int UartHalWriteBytes(const char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_HAL_UART_HAL_H_ */
