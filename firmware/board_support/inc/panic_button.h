#ifndef PANIC_BUTTON_H
#define PANIC_BUTTON_H

/** @defgroup board_support Board Support
 *  @brief Drivers de componentes externos de la placa.
 *  @{
 *  @defgroup panic_button Panic Button
 *  @brief Driver del pulsador de pánico (GPIO_PANIC_BTN, activo en bajo).
 *  @{
 *
 * @section genDesc General Description
 *
 * Este driver expone únicamente el estado crudo del pulsador de pánico y un
 * mecanismo de notificación por interrupción con debounce. No conoce ni
 * decide qué acción de negocio corresponde ante una pulsación: esa lógica
 * vive en la capa middleware (panic_handler).
 *
 *
 * @section changelog
 *
 * |   Date     | Description                                            |
 * |:----------:|:-------------------------------------------------------|
 * | 10/09/2026 | Reimplementado sobre gptimer_hal para debounce sin RTOS|
 *
 **/

/*==================[inclusions]=============================================*/
#include <stdbool.h>

/*==================[typedef]================================================*/

/**
 * @brief Firma para la función Callback de la Interrupción de Hardware (ISR).
 */
typedef void (*panic_button_isr_cb_t)(void *arg);

/*==================[external functions declaration]=========================*/

/**
 * @brief Inicializa el botón de pánico.
 *
 * Configura el GPIO como entrada y la base de tiempo (GpTimerHAL) usada
 * para el debounce. Idempotente.
 *
 * @return true si la inicialización fue exitosa.
 */
bool PanicButtonInit(void);

/**
 * @brief Consulta el estado actual del botón.
 *
 * @return true si el botón está presionado.
 */
bool PanicButtonIsPressed(void);

/**
 * @brief Asocia una rutina de interrupción de bajo nivel al flanco del GPIO.
 *
 * La ISR interna aplica debounce (30 ms) antes de invocar el callback
 * provisto: rebotes dentro de esa ventana se descartan silenciosamente.
 *
 * @param cb  Función callback a invocar en el contexto de la ISR.
 * @param arg Puntero o argumento opcional para la ISR (puede ser NULL).
 */
void PanicButtonAttachInterrupt(panic_button_isr_cb_t cb, void *arg);


/**
 * @brief Desactiva la interrupción del botón de pánico.
 *
 * A partir de esta llamada, el callback registrado con
 * PanicButtonAttachInterrupt() deja de invocarse ante cambios en el GPIO.
 * PanicButtonIsPressed() sigue funcionando con normalidad (es lectura
 * directa del pin, no depende de la interrupción).
 */
void PanicButtonDetachInterrupt(void);


#endif  /* PANIC_BUTTON_H */

/*==================[end of file]============================================*/