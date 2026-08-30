#include "status_indicator.h"

#include "led.h"


/*==================[macros and definitions]=================================*/

/**
 * @brief Período de ejecución de StatusIndicatorRunStep().
 */
#define STATUS_INDICATOR_STEP_MS       25U

/**
 * @brief Cantidad de pasos para cada intervalo.
 */
#define SEARCHING_ON_STEPS             (200U / STATUS_INDICATOR_STEP_MS)
#define SEARCHING_OFF_STEPS            (1800U / STATUS_INDICATOR_STEP_MS)

#define READY_ON_STEPS                 (1800U / STATUS_INDICATOR_STEP_MS)
#define READY_OFF_STEPS                (200U / STATUS_INDICATOR_STEP_MS)

#define TRANSMITTING_ON_STEPS          (125U / STATUS_INDICATOR_STEP_MS)
#define TRANSMITTING_OFF_STEPS         (125U / STATUS_INDICATOR_STEP_MS)


/*==================[internal data declaration]==============================*/

static cellular_status_t cellular_status = CELLULAR_STATUS_OFF;

static bool panic_active = false;

static uint32_t current_step = 0;


/*==================[external functions definition]==========================*/

bool StatusIndicatorInit(void)
{
    if (!LedInit())
    {
        return false;
    }

    cellular_status = CELLULAR_STATUS_OFF;
    panic_active = false;
    current_step = 0;

    return true;
}


void StatusIndicatorSetPanic(bool active)
{
    panic_active = active;

    if (panic_active)
    {
        LedOn(LED_PANIC);
    }
    else
    {
        LedOff(LED_PANIC);
    }
}


void StatusIndicatorSetCellular(cellular_status_t status)
{
    cellular_status = status;

    /*
     * Reiniciamos el patrón cada vez que cambia
     * el estado del módulo celular.
     */
    current_step = 0;

    switch (cellular_status)
    {
        case CELLULAR_STATUS_OFF:

            LedOff(LED_QUECTEL);

            break;


        case CELLULAR_STATUS_STARTING:

            /*
             * Durante el arranque el LED permanece encendido.
             */
            LedOn(LED_QUECTEL);

            break;


        case CELLULAR_STATUS_SEARCHING:

        case CELLULAR_STATUS_READY:

        case CELLULAR_STATUS_TRANSMITTING:

            /*
             * El patrón comienza con el LED encendido.
             * RunStep() continuará la secuencia.
             */
            LedOn(LED_QUECTEL);

            break;


        default:

            LedOff(LED_QUECTEL);
            current_step = 0;

            break;
    }
}


void StatusIndicatorRunStep(void)
{
    switch (cellular_status)
    {
        case CELLULAR_STATUS_OFF:

            LedOff(LED_QUECTEL);

            break;


        case CELLULAR_STATUS_STARTING:

            LedOn(LED_QUECTEL);

            break;


        case CELLULAR_STATUS_SEARCHING:

            /*
             * 200 ms ON / 1800 ms OFF
             */
            if (current_step < SEARCHING_ON_STEPS)
            {
                LedOn(LED_QUECTEL);
            }
            else
            {
                LedOff(LED_QUECTEL);
            }

            current_step++;

            if (current_step >=
                (SEARCHING_ON_STEPS + SEARCHING_OFF_STEPS))
            {
                current_step = 0;
            }

            break;


        case CELLULAR_STATUS_READY:

            /*
             * 1800 ms ON / 200 ms OFF
             */
            if (current_step < READY_ON_STEPS)
            {
                LedOn(LED_QUECTEL);
            }
            else
            {
                LedOff(LED_QUECTEL);
            }

            current_step++;

            if (current_step >=
                (READY_ON_STEPS + READY_OFF_STEPS))
            {
                current_step = 0;
            }

            break;


        case CELLULAR_STATUS_TRANSMITTING:

            /*
             * 125 ms ON / 125 ms OFF
             */
            if (current_step < TRANSMITTING_ON_STEPS)
            {
                LedOn(LED_QUECTEL);
            }
            else
            {
                LedOff(LED_QUECTEL);
            }

            current_step++;

            if (current_step >=
                (TRANSMITTING_ON_STEPS + TRANSMITTING_OFF_STEPS))
            {
                current_step = 0;
            }

            break;


        default:

            LedOff(LED_QUECTEL);
            current_step = 0;

            break;
    }
}
/**
 * @brief Obtiene el estado visual actual del módulo celular.
 *
 * @return Estado actual del indicador.
 */
cellular_status_t StatusIndicatorGetCellular(void)
{
    return cellular_status;
}