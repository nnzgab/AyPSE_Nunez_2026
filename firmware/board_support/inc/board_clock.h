#ifndef BOARD_CLOCK_H
#define BOARD_CLOCK_H

/** @defgroup board_support Board Support
 *  @brief Drivers de componentes externos de la placa.
 *  @{
 *  @defgroup board_clock Board Clock
 *  @brief Base de tiempo disponible para middleware.
 *  @{
 *
 * @section genDesc General Description
 *
 * Envoltorio de board_support sobre gptimer_hal (drivers_hal). Existe
 * únicamente para que middleware pueda pedir "cuántos ms pasaron" sin
 * incluir headers de drivers_hal directamente, respetando la adyacencia
 * estricta de capas (middleware -> board_support -> drivers_hal).
 *
 * No agrega lógica propia: es el único lugar de board_support autorizado
 * a incluir gptimer_hal.h para este fin.
 *
 
 *
 **/

/*==================[inclusions]=============================================*/
#include <stdbool.h>
#include <stdint.h>

/*==================[external functions declaration]=========================*/

/**
 * @brief Inicializa la base de tiempo. Idempotente.
 *
 * @return true si la inicialización fue exitosa.
 */
bool BoardClockInit(void);

/**
 * @brief Retorna el tiempo transcurrido desde la inicialización en
 * milisegundos.
 *
 * @return Milisegundos transcurridos (uint32_t).
 */
uint32_t BoardClockGetMs(void);

#endif /* #ifndef BOARD_CLOCK_H */

/*==================[end of file]============================================*/