#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

#include <stdbool.h>
#include <stdint.h>

/*==================[typedef]================================================*/

/**
 * @brief Estados visuales del módulo celular.
 */
typedef enum
{
    CELLULAR_STATUS_OFF = 0,
    CELLULAR_STATUS_STARTING,
    CELLULAR_STATUS_SEARCHING,
    CELLULAR_STATUS_READY,
    CELLULAR_STATUS_TRANSMITTING
} cellular_status_t;

/*==================[external functions declaration]=========================*/

/**
 * @brief Inicializa el middleware de indicadores.
 *
 * @return true si la inicialización fue correcta.
 */
bool StatusIndicatorInit(void);

/**
 * @brief Activa o desactiva el indicador de alarma.
 *
 * @param active true si existe una alarma pendiente.
 */
void StatusIndicatorSetPanic(bool active);

/**
 * @brief Establece el estado visual del módulo celular.
 *
 * @param status Estado actual del módulo celular.
 */
void StatusIndicatorSetCellular(cellular_status_t status);

/**
 * @brief Ejecuta un paso del patrón visual actual.
 *
 * Debe llamarse periódicamente desde la aplicación.
 */
void StatusIndicatorRunStep(void);



/**
 * @brief Obtiene el estado actual del indicador celular.
 *
 * @return Estado actual.
 */
cellular_status_t StatusIndicatorGetCellular(void);

#endif /* STATUS_INDICATOR_H */