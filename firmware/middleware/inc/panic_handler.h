#ifndef PANIC_HANDLER_H
#define PANIC_HANDLER_H

/** @defgroup middleware Middleware
 *  @brief Layer of intermediate logical services.
 *  @{
 *  @defgroup panic_handler Panic Handler Middleware
 *  @brief Middleware service for panic button event processing.
 *  @{
 * 
 * @section genDesc General Description
 * 
 * This middleware manages panic button events, applying non-blocking software debounce,
 * event sequence numbering, and invoking the registered user callback upon a validated panic event.
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
#include <stdint.h>
#include <stdbool.h>

/*==================[macros]=================================================*/

/*==================[typedef]================================================*/
/**
 * @brief Códigos de error devueltos por el módulo Panic Handler.
 */
typedef enum {
    PANIC_HANDLER_OK = 0,       /**< Inicialización exitosa */
    PANIC_HANDLER_ERR_PARAM,    /**< Error en los parámetros (ej. callback NULL) */
    PANIC_HANDLER_ERR_INIT      /**< Error al inicializar el hardware del botón */
} panic_handler_err_t;

/**
 * @brief Tipo de dato para el callback del evento de pánico.
 * @param sequence_number Número de secuencia del evento (se incrementa con cada pulsación).
 */
typedef void (*panic_event_cb_t)(uint16_t sequence_number);

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/
/**
 * @brief Inicializa el módulo Panic Handler.
 * 
 * Configura los datos de estado, registra el callback de evento del usuario y
 * conecta la interrupción física del hardware.
 * 
 * @param callback Función callback a registrar ante un evento de pánico.
 * @return panic_handler_err_t Código de error o éxito de la inicialización.
 */
panic_handler_err_t PanicHandler_Init(panic_event_cb_t callback);

/**
 * @brief Función de pasada no bloqueante (RunStep).
 * 
 * Procesa la máquina de estados no bloqueante para reconfirmación de pulsación y debounce.
 * Debe invocarse periódicamente desde el ciclo principal de la aplicación.
 */
void PanicHandler_RunStep(void);

/** @} */
/** @} */

#endif /* #ifndef PANIC_HANDLER_H */

/*==================[end of file]============================================*/