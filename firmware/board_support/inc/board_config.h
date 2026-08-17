#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "gpio_hal.h"
/*==================[inclusions]=============================================*/
#include <stdint.h>   /* solo headers estándar en headers públicos */

/*==================[macros]=================================================*/


/** @name Configuración UART1 (Módulo Quectel EG915U-LA)
  * @{ */
#define UART1_TX_PIN        GPIO_18
#define UART1_RX_PIN        GPIO_19

/* Mapeo hacia el HAL */
#define UART_HAL_TX_PIN     UART1_TX_PIN
#define UART_HAL_RX_PIN     UART1_RX_PIN
#define UART_BAUDRATE       115200
/** @} */

/* Mapeo de periféricos físicos del Gateway */
#define GPIO_PANIC_BTN              GPIO_23  /* Entrada digital para botón de pánico */
#define GPIO_PANIC_LED_STATUS       GPIO_4   /* LED indicador de estado para botón de pánico */
#define GPIO_QUECTEL_LED_STATUS     GPIO_5   /* LED indicador de estado de conectividad de red */


/* Pines del módulo Quectel (ejemplo) */
#define QUECTEL_PWRKEY_PIN  GPIO_6  /* salida digital para encender el modulo */

/*==================[typedef]================================================*/

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/



#endif /* BOARD_CONFIG_H */

/*==================[end of file]============================================*/
