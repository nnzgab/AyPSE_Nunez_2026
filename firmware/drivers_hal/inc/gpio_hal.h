#ifndef GPIO_HAL_H
#define GPIO_HAL_H

/** @defgroup hal HAL
 *  @brief Hardware Abstraction Layer.
 *  @{
 *  @defgroup gpio_hal GPIO HAL
 *  @brief GPIO driver for ESP32-C6.
 *  @{
 * 
 * @section genDesc General Description
 *
 * This driver provide functions to configure and handle the ESP32-C6 General
 * Purpose Input-Outputs.
 * 
 * @note GPIO_12 and GPIO_13 are not recommended for use, because using them will
 * overwrite the flash and debug functionalities via USB.
 * 
 * @note GPIO_16 and GPIO_17 are not recommended for use, because using them will
 * overwrite the flash and debug functionalities via UART.
 * 
 * @author Albano Peñalva
 *
 * @section changelog
 *
 * |   Date	    | Description                                    						|
 * |:----------:|:----------------------------------------------------------------------|
 * | 23/10/2023 | Document creation		                         						|
 * | 18/06/2026 | Update documentation		                         					|
 * 
 **/

/*==================[inclusions]=============================================*/
#include <stdbool.h>
#include <stdint.h>
//#include "esp_err.h"
/*==================[macros]=================================================*/

/*==================[typedef]================================================*/
/**
 * @brief GPIO direction (input or output). Modo de funcionamiento
 * 
 */
typedef enum {
	GPIO_INPUT = 0, 	/**< Input with pull-up resistor */
	GPIO_OUTPUT			/**< Output */
	} io_t;

/**
 * @brief Tipo de interrupció.
 * 
 */
typedef enum
{
    GPIO_INT_DISABLE,
    GPIO_INT_RISING,
    GPIO_INT_FALLING,
    GPIO_INT_BOTH
} gpio_int_mode_t;

/* Callback de interrupción */
typedef void (*gpio_callback_t)(void *args);


/**
 * @brief ESP32-C6 available GPIOs (not all of them are available)
 * 
 */
typedef enum gpio_list{
	GPIO_0=0, 	/**< GPIO0 */
	GPIO_1, 	/**< GPIO1 */
	GPIO_2, 	/**< GPIO2 */
	GPIO_3, 	/**< GPIO3 */
	GPIO_4, 	/**< GPIO4 */
	GPIO_5, 	/**< GPIO5 */
	GPIO_6, 	/**< GPIO6 */
	GPIO_7, 	/**< GPIO7 */
	GPIO_8, 	/**< GPIO8 - shared with NeoPixel */
	GPIO_9, 	/**< GPIO9 */
	GPIO_10, 	/**< GPIO10 */
	GPIO_11, 	/**< GPIO11 */
	GPIO_12, 	/**< GPIO12 - shared with USB_D- */
	GPIO_13, 	/**< GPIO13 - shared with USB_D+ */
	GPIO_14, 	/**< not available */
	GPIO_15, 	/**< GPIO15 */
	GPIO_16, 	/**< GPIO16 - shared with UART_PC TX */
	GPIO_17, 	/**< GPIO17 - shared with UART_PC RX */
	GPIO_18, 	/**< GPIO18 */
	GPIO_19, 	/**< GPIO19 */
	GPIO_20, 	/**< GPIO20 */
	GPIO_21,	/**< GPIO21  */
	GPIO_22, 	/**< GPIO22  */
	GPIO_23, 	/**< GPIO23 */
} gpio_t;


/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/
/**
 * @brief GPIO initialization
 * 
 * @param pin GPIO number
 * @param io GPIO direction
 */
void GPIOInit(gpio_t pin, io_t io);

/* Operaciones sobre salidas */

/**
 * @brief Change GPIO state to high
 * 
 * @param pin GPIO number
 */
void GPIOOn(gpio_t pin);

/**
 * @brief Change GPIO state to low
 * 
 * @param pin GPIO number
 */
void GPIOOff(gpio_t pin);

/**
 * @brief Change GPIO state
 * 
 * @param pin GPIO number
 * @param state GPIO state (true: high - false: low)
 */
void GPIOState(gpio_t pin, bool state);

/**
 * @brief Invert GPIO state
 * 
 * @param pin GPIO number
 */
void GPIOToggle(gpio_t pin);


/* Lectura de entrada */
/**
 * @brief Reads GPIO state
 * 
 * @param pin 
 * @return true GPIO input high
 * @return false GPIO input low
 */
bool GPIORead(gpio_t pin);


/**
 * @brief Configure GPIO input interruption esto rescribir
 * 
 * @param pin GPIO number
 * @param ptr_int_func Pointer to callback function
 * @param edge true: positive edge - false: negative edge
 * @param args 
 */


/* Interrupciones */
void GPIOActivInt(
    gpio_t pin,
    gpio_int_mode_t mode,
    gpio_callback_t callback,
    void *args
);




/**
 * @brief que va aca desinstalacion?
 *
 * @param pin GPIO number
 */
/* Desinicialización */
void GPIODeactivInt(gpio_t pin);


#endif /* #ifndef GPIO_HAL_H */

/*==================[end of file]============================================*/
