#ifndef DRIVERS_HAL_UART_HAL_H_
#define DRIVERS_HAL_UART_HAL_H_

/** @defgroup hal HAL
 *  @brief Hardware Abstraction Layer.
 *  @{
 *  @defgroup uart_hal UART HAL
 *  @brief UART Hardware Abstraction Layer driver.
 *  @{
 *  @section genDesc General Description
 *  Header file for the UART Hardware Abstraction Layer module.
 *  @author Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)
 *  @section changelog
 *  |   Date     | Description                                            |
 *  |:----------:|:-------------------------------------------------------|
 *  | 20/10/2023 | Document creation                                      |
 */

/*==================[inclusions]=============================================*/
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================[macros and definitions]=================================*/
#define HAL_UART_OK     0
#define HAL_UART_ERROR -1

/*==================[typedef]================================================*/

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/
void UartHalInitWithPins(int baud, int tx_pin, int rx_pin);

void UartHalInit(int baud);

int UartHalReadByte(char *rcv);

int UartHalReadBytes(char *buf, size_t len, uint32_t timeout_ms);

/** Escribe un byte (no bloqueante) */
void UartHalWriteByte(char tx);

/** Escribe len bytes; espera a que termine la transmisión */
int UartHalWriteBytes(const char *buf, size_t len);

void HalDelayMs(uint32_t ms);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_HAL_UART_HAL_H_ */

/*==================[end of file]============================================*/