#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

/** @defgroup middleware Middleware
 *  @brief Layer of intermediate logical services.
 *  @{
 *  @defgroup status_indicator Status Indicator Middleware
 *  @brief Service for managing cellular network and panic status LED indicators.
 *  @{
 * 
 * @section genDesc General Description
 * 
 * This middleware manages the status indication logic using LEDs (LED_QUECTEL and LED_PANIC).
 * It provides non-blocking pattern execution for searching, ready, and transmitting states,
 * as well as panic status indication.
 * 
 * @author Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)
 *
 * @section changelog
 *
 * |   Date     | Description                                                            |
 * |:----------:|:----------------------------------------------------------------------|
 * | 20/09/2026 | Document creation and initial implementation                          |
 * 
 **/

/*==================[inclusions]=============================================*/
#include <stdbool.h>
#include <stdint.h>

/*==================[macros]=================================================*/

/*==================[typedef]================================================*/
/**
 * @brief Cellular status enumeration for LED blink pattern control.
 */
typedef enum {
    CELLULAR_STATUS_OFF = 0,       /**< LED off */
    CELLULAR_STATUS_STARTING,      /**< Fixed LED on */
    CELLULAR_STATUS_SEARCHING,     /**< Blink pattern: 900 ms ON / 100 ms OFF */
    CELLULAR_STATUS_READY,         /**< Blink pattern: 100 ms ON / 900 ms OFF */
    CELLULAR_STATUS_TRANSMITTING   /**< Fast blink pattern: 125 ms ON / 125 ms OFF */
} cellular_status_t;

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/
/**
 * @brief Initialize the status indicator middleware.
 * 
 * Initializes underlying LED and clock BSP drivers and sets initial LED states.
 * 
 * @return true if initialization was successful, false otherwise.
 */
bool StatusIndicator_Init(void);

/**
 * @brief Sets the cellular status and updates the LED pattern.
 * 
 * @param status Cellular status mode to set.
 */
void StatusIndicator_SetCellular(cellular_status_t status);

/**
 * @brief Gets the current cellular status.
 * 
 * @return cellular_status_t Current cellular status.
 */
cellular_status_t StatusIndicator_GetCellular(void);

/**
 * @brief Controls the panic status LED.
 * 
 * @param active true to turn on panic LED, false to turn off.
 */
void StatusIndicator_SetPanic(bool active);

/**
 * @brief Advances the non-blocking LED blinking state machine.
 * 
 * Must be called periodically from the main application loop.
 */
void StatusIndicator_RunStep(void);

/** @} */
/** @} */

#endif /* #ifndef STATUS_INDICATOR_H */

/*==================[end of file]============================================*/