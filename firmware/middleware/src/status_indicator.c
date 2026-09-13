/*==================[inclusions]=============================================*/
#include "status_indicator.h"
#include "led.h"          /* Capa BSP: LedInit, LedOn, LedOff */
#include "board_clock.h"  /* Capa BSP: BoardClockInit, BoardClockGetMs */

/*==================[macros and definitions]==================================*/
/* Tiempos exactos originales en milisegundos */
#define SEARCHING_ON_MS       200U
#define SEARCHING_OFF_MS      1800U

#define READY_ON_MS           1800U
#define READY_OFF_MS          200U

#define TRANSMITTING_ON_MS    125U
#define TRANSMITTING_OFF_MS   125U

/*==================[internal data definition]=================================*/
static cellular_status_t s_cellular_status = CELLULAR_STATUS_OFF;
static bool              s_panic_active    = false;
static bool              s_led_state       = false;
static uint32_t          s_last_toggle_ms  = 0;

/*==================[internal functions definition]==============================*/

/**
 * @brief Duración (ms) del semiciclo "encendido" para el estado dado.
 */
static uint32_t GetOnMs(cellular_status_t status)
{
    switch (status) {
        case CELLULAR_STATUS_SEARCHING:     return SEARCHING_ON_MS;
        case CELLULAR_STATUS_READY:         return READY_ON_MS;
        case CELLULAR_STATUS_TRANSMITTING:  return TRANSMITTING_ON_MS;
        default:                            return 0U;
    }
}

/**
 * @brief Duración (ms) del semiciclo "apagado" para el estado dado.
 */
static uint32_t GetOffMs(cellular_status_t status)
{
    switch (status) {
        case CELLULAR_STATUS_SEARCHING:     return SEARCHING_OFF_MS;
        case CELLULAR_STATUS_READY:         return READY_OFF_MS;
        case CELLULAR_STATUS_TRANSMITTING:  return TRANSMITTING_OFF_MS;
        default:                            return 0U;
    }
}

/*==================[external functions definition]=============================*/
bool StatusIndicator_Init(void)
{
    if (!LedInit()) {
        return false;
    }

    if (!BoardClockInit()) {
        return false;
    }

    s_cellular_status = CELLULAR_STATUS_OFF;
    s_panic_active = false;
    s_led_state = false;
    s_last_toggle_ms = BoardClockGetMs();

    LedOff(LED_QUECTEL);
    LedOff(LED_PANIC);

    return true;
}

void StatusIndicator_SetPanic(bool active)
{
    s_panic_active = active;

    if (s_panic_active) {
        LedOn(LED_PANIC);
    } else {
        LedOff(LED_PANIC);
    }
}

void StatusIndicator_SetCellular(cellular_status_t status)
{
    if (s_cellular_status == status) {
        return;
    }

    s_cellular_status = status;
    s_last_toggle_ms = BoardClockGetMs();

    switch (s_cellular_status) {
        case CELLULAR_STATUS_OFF:
            s_led_state = false;
            LedOff(LED_QUECTEL);
            break;

        case CELLULAR_STATUS_STARTING:
            /* Fijo, no parpadea */
            s_led_state = true;
            LedOn(LED_QUECTEL);
            break;

        case CELLULAR_STATUS_SEARCHING:
        case CELLULAR_STATUS_READY:
        case CELLULAR_STATUS_TRANSMITTING:
            /* Arranca el patrón siempre en ON; RunStep se encarga del resto */
            s_led_state = true;
            LedOn(LED_QUECTEL);
            break;

        default:
            s_led_state = false;
            LedOff(LED_QUECTEL);
            break;
    }
}

cellular_status_t StatusIndicator_GetCellular(void)
{
    return s_cellular_status;
}

void StatusIndicator_RunStep(void)
{
    /* OFF y STARTING no parpadean: nada que hacer */
    if ((s_cellular_status != CELLULAR_STATUS_SEARCHING) &&
        (s_cellular_status != CELLULAR_STATUS_READY) &&
        (s_cellular_status != CELLULAR_STATUS_TRANSMITTING)) {
        return;
    }

    uint32_t now = BoardClockGetMs();
    uint32_t elapsed = now - s_last_toggle_ms;
    uint32_t target_ms = s_led_state ? GetOnMs(s_cellular_status)
                                      : GetOffMs(s_cellular_status);

    if (elapsed >= target_ms) {
        s_led_state = !s_led_state;

        if (s_led_state) {
            LedOn(LED_QUECTEL);
        } else {
            LedOff(LED_QUECTEL);
        }

        s_last_toggle_ms = now;
    }
}

/*==================[end of file]============================================*/