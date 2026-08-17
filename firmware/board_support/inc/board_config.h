#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/** @defgroup bsp BSP
 *  @brief Board support package layer.
 *  @{
 *  @defgroup board_config Board Configuration
 *  @brief Pin mapping and board configuration
 *  @{
 */

/*==================[inclusions]=============================================*/
#include <stdint.h>   /* solo headers estándar en headers públicos */
/*==================[macros]=================================================*/
/** @name Mapeo de Pines de LEDs
 *  @{
 */
#define GPIO_LED1   4  /**< LED 1 (Green) */
#define GPIO_LED2   5  /**< LED 2 (Yellow) */
#define GPIO_LED3   6  /**< LED 3 (Red) */
/** @} */

/** @name Pines UART1 (HAL)
 *  @{
 *  TX = GPIO 18, RX = GPIO 19 (decidido previamente)
 */
#define UART1_TX_PIN  18    /**< UART1 TX */
#define UART1_RX_PIN  19    /**< UART1 RX */

/* Alias que puede usar el HAL (opcional, evita acoplamientos de nombres) */
#define UART_HAL_TX_PIN  UART1_TX_PIN
#define UART_HAL_RX_PIN  UART1_RX_PIN
/** @} */


/* Pines del módulo Quectel (ejemplo) */
#define QUECTEL_PWRKEY_PIN  21  /////////////////////////////////

/*==================[typedef]================================================*/

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/



#endif /* BOARD_CONFIG_H */

/*==================[end of file]============================================*/
