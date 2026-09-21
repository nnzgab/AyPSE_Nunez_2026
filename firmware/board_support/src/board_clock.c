/**
 * @file board_clock.c
 * @author Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)
 * @brief Board Clock driver implementation.
 * @version 0.1
 * @date 2026-09-03
 * @copyright Copyright (c) 2026
 */

/*==================[inclusions]=============================================*/
#include "board_clock.h"
#include "gptimer_hal.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/
bool BoardClockInit(void) {
    return (GpTimerInit() == HAL_GPTIMER_OK);
}

uint32_t BoardClockGetMs(void) {
    return GpTimerGetMs();
}

void BoardClockDelayMs(uint32_t ms) {
    GpTimerDelayMs(ms);
}

/*==================[end of file]============================================*/