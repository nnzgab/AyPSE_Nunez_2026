// firmware/board_support/src/panic_button.c
#include "panic_button.h"
#include "gpio_hal.h"
#include "board_config.h"

bool PanicButtonInit(void) {
    // Aquí utilizas el macro del board_config
    GPIOInit(GPIO_PANIC_BTN, GPIO_INPUT);
    return true;
}

bool PanicButtonIsPressed(void) {
    return !GPIORead(GPIO_PANIC_BTN);
}