#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CELLULAR_STATUS_OFF = 0,
    CELLULAR_STATUS_STARTING,
    CELLULAR_STATUS_SEARCHING,     /* 200 ms ON  / 1800 ms OFF */
    CELLULAR_STATUS_READY,         /* 500 ms ON / 500 ms OFF  */ /* 1800 ms ON / 200 ms OFF  */
    CELLULAR_STATUS_TRANSMITTING   /* 125 ms ON  / 125 ms OFF  */
} cellular_status_t;

bool StatusIndicator_Init(void);
void StatusIndicator_SetCellular(cellular_status_t status);
cellular_status_t StatusIndicator_GetCellular(void);
void StatusIndicator_SetPanic(bool active);

/**
 * @brief Avanza la máquina de parpadeo de LED_QUECTEL.
 *
 * Debe llamarse periódicamente desde el loop de la app (por ejemplo cada
 * 10-50 ms). No bloquea: en cada llamada compara el tiempo transcurrido
 * desde el último cambio de estado del LED contra el tiempo objetivo del
 * patrón actual (según CELLULAR_STATUS_SEARCHING/READY/TRANSMITTING) y
 * conmuta el LED solo si corresponde.
 */
void StatusIndicator_RunStep(void);

#endif /* STATUS_INDICATOR_H */