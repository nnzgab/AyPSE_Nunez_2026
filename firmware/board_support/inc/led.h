#ifndef LED_H
#define LED_H

#include <stdbool.h>

/*==================[typedef]================================================*/
typedef enum {
    LED_PANIC,
    LED_QUECTEL
} led_id_t;

/*==================[external functions declaration]=========================*/
bool LedInit(void);
void LedOn(led_id_t id);
void LedOff(led_id_t id);
void LedToggle(led_id_t id);
void LedState(led_id_t id, bool state);


#endif /* LED_H */