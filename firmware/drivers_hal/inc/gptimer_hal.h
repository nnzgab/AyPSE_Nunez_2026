#ifndef DRIVERS_HAL_GPTIMER_HAL_H_
#define DRIVERS_HAL_GPTIMER_HAL_H_

/** @defgroup hal HAL
 *  @brief Hardware Abstraction Layer.
 *  @{
 *  @defgroup gptimer_hal GPTimer HAL
 *  @brief General Purpose Timer Hardware Abstraction Layer driver.
 *  @{
 *  @section genDesc General Description
 *  Header file for the General Purpose Timer Hardware Abstraction Layer module.
 *  @author Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)
 *  @section changelog
 *  |   Date     | Description                                            |
 *  |:----------:|:-------------------------------------------------------|
 *  | 20/10/2023 | Document creation                                      |
 */

/*==================[inclusions]=============================================*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================[macros and definitions]=================================*/
#define HAL_GPTIMER_OK     0
#define HAL_GPTIMER_ERROR -1

/*==================[typedef]================================================*/

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/
/**
 * @brief Inicializa el GPTimer con resolución de 1 µs.
 * si ya fue inicializado retorna HAL_GPTIMER_OK sin
 * reconfigurar ni reiniciar el conteo.
 * @return HAL_GPTIMER_OK en éxito, HAL_GPTIMER_ERROR en fallo.
 */
int8_t GpTimerInit(void);

/**
 * @brief Retorna el tiempo transcurrido desde la inicialización en
 * milisegundos.
 * @return Milisegundos transcurridos (uint32_t).
 */
uint32_t GpTimerGetMs(void);

/**
 * @brief Espera bloqueante (busy-wait) por la cantidad de milisegundos
 * indicada.
 * No cede el CPU al scheduler. Adecuado solo para retardos cortos durante
 * inicialización (por ejemplo, el pulso de PWRKEY del módulo celular).
 * No debe usarse dentro de una ISR ni en el camino del botón de pánico.
 * @param ms Milisegundos a esperar (0 retorna inmediatamente).
 */
void GpTimerDelayMs(uint32_t ms);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_HAL_GPTIMER_HAL_H_ */

/*==================[end of file]============================================*/