#ifndef PANIC_HANDLER_H
#define PANIC_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Códigos de error devueltos por el módulo Panic Handler
 */
typedef enum {
    PANIC_HANDLER_OK = 0,       /*!< Inicialización exitosa */
    PANIC_HANDLER_ERR_PARAM,    /*!< Error en los parámetros (ej. callback NULL) */
    PANIC_HANDLER_ERR_INIT      /*!< Error al inicializar el hardware del botón */
} panic_handler_err_t;

/**
 * @brief Tipo de dato para el callback del evento de pánico.
 * 
 * @param sequence_number Número de secuencia del evento (se incrementa con cada pulsación)
 */
typedef void (*panic_event_cb_t)(uint16_t sequence_number);

/**
 * @brief Inicializa el hardware del botón y registra el callback.
 * 
 * @param callback Función que será ejecutada cuando se detecte una pulsación válida.
 * @return panic_handler_err_t PANIC_HANDLER_OK si fue exitoso, o el código de error correspondiente.
 */
panic_handler_err_t PanicHandler_Init(panic_event_cb_t callback);

/**
 * @brief Máquina de estados no bloqueante del handler de pánico.
 *        Debe ser llamada periódicamente desde el bucle principal (super-loop) 
 *        o desde una tarea de RTOS para procesar los rebotes y disparar el callback.
 */
void PanicHandler_RunStep(void);

#endif /* PANIC_HANDLER_H */