/**
 * @file uart_hal.h
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

/** \brief UART HAL driver for the ESP32.
 *
 * This file is the template for developing a new Hardware Abstraction Layer (HAL)
 * driver. It wraps the low-level ESP-IDF peripheral API and exposes it through
 * a hardware-independent interface.
 *
 * @author Gabriel
 *
 * @section changelog
 *
 * |   Date     | Description                                            |
 * |:----------:|:-------------------------------------------------------|
 * | 13/08/2026 | Initial creation following course template             |
 *
 **/

/*==================[inclusions]=============================================*/
#ifndef UART_HAL_H
#define UART_HAL_H

#include <stdint.h>
#include <stdbool.h>
/*==================[macros]=================================================*/

/*==================[typedef]================================================*/
/**
 * @brief UART HAL configuration structure
 */
typedef struct {
    int uart_num;        /*!< UART port number (e.g., 0,1,2) */
    int tx_pin;          /*!< TX GPIO number */
    int rx_pin;          /*!< RX GPIO number */
    uint32_t baudrate;   /*!< Baud rate */
    int rx_buffer_size;  /*!< RX buffer size in bytes */
    int tx_buffer_size;  /*!< TX buffer size in bytes */
} uart_hal_cfg_t;

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/
/**
 * @brief Initialize the UART peripheral according to cfg
 * @param cfg Pointer to configuration structure
 * @return uint8_t true on success, false on failure
 */
uint8_t UartHalInit(const uart_hal_cfg_t *cfg);

/**
 * @brief Write bytes to UART (blocking for the driver call)
 * @param data Pointer to data buffer
 * @param len Number of bytes to write
 * @param timeout_ms Timeout in milliseconds for driver wait (if used)
 * @return int Number of bytes written
 */
int UartHalWrite(const uint8_t *data, size_t len, uint32_t timeout_ms);

/**
 * @brief Read bytes from UART
 * @param buf Destination buffer
 * @param len Maximum bytes to read
 * @param timeout_ms Timeout in milliseconds
 * @return int Number of bytes read
 */
int UartHalRead(uint8_t *buf, size_t len, uint32_t timeout_ms);

/**
 * @brief Change UART baudrate at runtime
 * @param baudrate New baudrate
 * @return uint8_t true on success
 */
uint8_t UartHalSetBaud(uint32_t baudrate);

/**
 * @brief Install a receive callback (optional)
 * @param cb Callback function pointer (NULL to uninstall)
 * @param arg User argument passed to callback
 * @return uint8_t true on success
 */
typedef void (*uart_rx_cb_t)(const uint8_t *data, size_t len, void *arg);
uint8_t UartHalInstallRxCb(uart_rx_cb_t cb, void *arg);

/**
 * @brief Deinitialize UART HAL
 */
void UartHalDeinit(void);

#endif /* UART_HAL_H */

/** @} */ /* end of defgroup hal */

/*==================[end of file]============================================*/
