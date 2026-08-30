#ifndef PANIC_HANDLER_H
#define PANIC_HANDLER_H

#include <stdbool.h>

/*==================[typedef]================================================*/

/**
 * @brief Estados de la alarma de pánico.
 */
typedef enum
{
    PANIC_STATUS_NORMAL = 0, /**< No hay alarma pendiente */
    PANIC_STATUS_PENDING     /**< Hay una alarma pendiente */
} panic_status_t;


/*==================[external functions declaration]=========================*/

/**
 * @brief Inicializa el manejador de pánico.
 *
 * Inicializa el botón de pánico y coloca el estado
 * del manejador en NORMAL.
 *
 * @return true si la inicialización fue correcta.
 */
bool PanicHandlerInit(void);


/**
 * @brief Procesa los eventos del botón de pánico.
 *
 * Debe ejecutarse periódicamente desde la aplicación.
 */
void PanicHandlerRunStep(void);


/**
 * @brief Obtiene el estado actual de la alarma.
 *
 * @return Estado actual de la alarma.
 */
panic_status_t PanicHandlerGetStatus(void);


/**
 * @brief Indica si existe una alarma pendiente.
 *
 * @return true si hay una alarma pendiente.
 */
bool PanicHandlerIsActive(void);


/**
 * @brief Limpia la alarma pendiente.
 *
 * Por ahora representa que la alarma fue atendida.
 * Más adelante podrá utilizarse luego de recibir
 * la confirmación del servidor.
 */
void PanicHandlerClear(void);

#endif /* PANIC_HANDLER_H */