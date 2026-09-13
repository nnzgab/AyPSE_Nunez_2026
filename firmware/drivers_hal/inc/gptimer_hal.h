#ifndef GPTIMER_HAL_H
#define GPTIMER_HAL_H

/** @defgroup hal HAL
 *  @brief Hardware Abstraction Layer.
 *  @{
 *  @defgroup gptimer_hal GPTimer HAL
 *  @brief General purpose timer driver for ESP32-C6.
 *  @{
 *
 * @section genDesc General Description
 *
 * Provee una base de tiempo en milisegundos para las capas superiores,
 * ocultando el uso del driver gptimer de ESP-IDF. Ninguna capa por encima
 * de drivers_hal debe incluir <driver/gptimer.h> directamente.
 *
 * @note El contador satura (overflow) luego de aproximadamente 49 días
 * de operación continua (uint32_t en milisegundos). Las comparaciones de
 * tiempo transcurrido deben hacerse por resta (unsigned wraparound),
 * nunca comparando valores absolutos.
 *
 * 
 * @section changelog
 *
 * |   Date     | Description                                            |
 * |:----------:|:-------------------------------------------------------|
 * |  | Document creation                                      |
 *
 **/

/*==================[inclusions]=============================================*/
#include <stdint.h>

/*==================[macros]=================================================*/
#define HAL_GPTIMER_OK      (0)   /**< Operación exitosa */
#define HAL_GPTIMER_ERROR   (-1)  /**< Error genérico */

/*==================[typedef]================================================*/

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/

/**
 * @brief Inicializa el GPTimer con resolución de 1 µs.
 *
 * Idempotente: si ya fue inicializado retorna HAL_GPTIMER_OK sin
 * reconfigurar ni reiniciar el conteo.
 *
 * @return HAL_GPTIMER_OK en éxito, HAL_GPTIMER_ERROR en fallo.
 */
int8_t GpTimerInit(void);

/**
 * @brief Retorna el tiempo transcurrido desde la inicialización en
 * milisegundos.
 *
 * @return Milisegundos transcurridos (uint32_t).
 */
uint32_t GpTimerGetMs(void);

/**
 * @brief Espera bloqueante (busy-wait) por la cantidad de milisegundos
 * indicada.
 *
 * No cede el CPU al scheduler. Adecuado solo para retardos cortos durante
 * inicialización (por ejemplo, el pulso de PWRKEY del módulo celular).
 * No debe usarse dentro de una ISR ni en el camino del botón de pánico.
 *
 * @param ms Milisegundos a esperar (0 retorna inmediatamente).
 */
void GpTimerDelayMs(uint32_t ms);

#endif /* #ifndef GPTIMER_HAL_H */

/*==================[end of file]============================================*/