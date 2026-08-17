#include "led.h"
#include "gpio_hal.h"
#include "board_config.h"

bool LedInit(void) {
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);
    GPIOInit(GPIO_QUECTEL_LED_STATUS, GPIO_OUTPUT);
    
    GPIOOff(GPIO_PANIC_LED_STATUS);
    GPIOOff(GPIO_QUECTEL_LED_STATUS);
    return true;
}

void LedOn(led_id_t id) {
    if (id == LED_PANIC) {
        GPIOOn(GPIO_PANIC_LED_STATUS);
    } else if (id == LED_QUECTEL) {
        GPIOOn(GPIO_QUECTEL_LED_STATUS);
    }
}

void LedOff(led_id_t id) {
    if (id == LED_PANIC) {
        GPIOOff(GPIO_PANIC_LED_STATUS);
    } else if (id == LED_QUECTEL) {
        GPIOOff(GPIO_QUECTEL_LED_STATUS);
    }
}

void LedToggle(led_id_t id) {
    if (id == LED_PANIC) {
        GPIOToggle(GPIO_PANIC_LED_STATUS);
    } else if (id == LED_QUECTEL) {
        GPIOToggle(GPIO_QUECTEL_LED_STATUS);
    }
}

void LedState(led_id_t id, bool state) {
    if (id == LED_PANIC) {
        GPIOState(GPIO_PANIC_LED_STATUS, state);
    } else if (id == LED_QUECTEL) {
        GPIOState(GPIO_QUECTEL_LED_STATUS, state);
    }
}