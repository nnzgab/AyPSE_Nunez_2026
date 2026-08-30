#include "panic_handler.h"

#include "panic_button.h"


/*==================[internal data declaration]==============================*/

/**
 * @brief Estado actual del manejador de pánico.
 */
static panic_status_t panic_status = PANIC_STATUS_NORMAL;


/*==================[external functions definition]===========================*/

bool PanicHandlerInit(void)
{
    /*
     * Comenzamos siempre sin una alarma pendiente.
     */
    panic_status = PANIC_STATUS_NORMAL;

    /*
     * Inicializamos el botón de pánico.
     *
     * La implementación del botón pertenece al BSP.
     */
    if (!PanicButtonInit())
    {
        return false;
    }

    return true;
}


void PanicHandlerRunStep(void)
{
    /*
     * Solamente buscamos una nueva pulsación cuando
     * no existe una alarma pendiente.
     */
    if (panic_status == PANIC_STATUS_NORMAL)
    {
        /*
         * PanicButtonPressedEvent() devuelve true
         * cuando el BSP detectó una pulsación válida.
         */
        if (PanicButtonPressedEvent())
        {
            panic_status = PANIC_STATUS_PENDING;
        }
    }
}


panic_status_t PanicHandlerGetStatus(void)
{
    return panic_status;
}


bool PanicHandlerIsActive(void)
{
    return (panic_status == PANIC_STATUS_PENDING);
}


void PanicHandlerClear(void)
{
    /*
     * Por ahora simplemente eliminamos la alarma pendiente.
     *
     * Más adelante esta función podrá ser llamada
     * cuando Cellular confirme que el servidor recibió
     * correctamente la alarma.
     */
    panic_status = PANIC_STATUS_NORMAL;
}


/*==================[end of file]============================================*/