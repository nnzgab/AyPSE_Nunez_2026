/**
 * @file panic_button.c
 * @author Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)
 * @brief Panic Button BSP driver implementation.
 * @version 0.1
 * @date 2026-09-03
 * @copyright Copyright (c) 2026
 */


/*==================[inclusions]=============================================*/
#include "panic_button.h"
#include "board_config.h"
#include "gpio_hal.h"
#include "gptimer_hal.h"

/*==================[macros and definitions]==================================*/
#define PANIC_DEBOUNCE_MS  30U

/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]=================================*/
static panic_button_isr_cb_t user_cb = NULL;
static void *user_arg = NULL;
static volatile uint32_t last_event_ms = 0;

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/**
 * @brief ISR interna del pulsador de pánico.
 *
 * Aplica debounce por comparación de timestamps (resta con wraparound,
 * válida aun si GpTimerGetMs() da la vuelta) y, si corresponde, invoca el
 * callback del usuario. Debe mantenerse corta: no hace nada más que esto.
 */
static void PanicButtonInternalIsr(void *arg)
{
    uint32_t now = GpTimerGetMs();
    if ((now - last_event_ms) < PANIC_DEBOUNCE_MS) {
        return; /* rebote dentro de la ventana de debounce: se descarta */
    }
    last_event_ms = now;
    if (user_cb != NULL) {
        user_cb(user_arg);
    }
}

/*==================[external functions definition]=============================*/
bool PanicButtonInit(void)
{
    GPIOInit(GPIO_PANIC_BTN, GPIO_INPUT);
    return (GpTimerInit() == HAL_GPTIMER_OK);
}

bool PanicButtonIsPressed(void)
{
    return !GPIORead(GPIO_PANIC_BTN); /* activo en bajo */
}

void PanicButtonAttachInterrupt(panic_button_isr_cb_t cb, void *arg)
{
    user_cb = cb;
    user_arg = arg;
    GPIOActivInt(GPIO_PANIC_BTN, GPIO_INT_FALLING, PanicButtonInternalIsr, NULL);
}


void PanicButtonDetachInterrupt(void)
{
    GPIODeactivInt(GPIO_PANIC_BTN);
    user_cb = NULL;
    user_arg = NULL;
}


/*==================[end of file]============================================*/