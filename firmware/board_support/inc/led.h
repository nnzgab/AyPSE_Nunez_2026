#ifndef LED_H
#define LED_H

#include <stdbool.h>

/*==================[typedef]================================================*/
typedef enum {
    LED_PANIC,
    LED_QUECTEL
} led_id_t;

/*==================[external functions declaration]=========================*/

/**
 * @brief Inicializa los LEDs de la placa.
 *
 * @return true si la inicialización fue correcta.
 */
bool LedInit(void);


/**
 * @brief Enciende un LED.
 *
 * @param id Identificador del LED.
 */
void LedOn(led_id_t id);


/**
 * @brief Apaga un LED.
 *
 * @param id Identificador del LED.
 */
void LedOff(led_id_t id);

/**
 * @brief Invierte el estado de un LED.
 *
 * @param id Identificador del LED.
 */
void LedToggle(led_id_t id);


/**
 * @brief Establece el estado de un LED.
 *
 * @param id Identificador del LED.
 * @param state true = encendido, false = apagado.
 */
void LedState(led_id_t id, bool state);


#endif /* LED_H */