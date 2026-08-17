#ifndef PANIC_BUTTON_H
#define PANIC_BUTTON_H

#include <stdbool.h>

/**
 * @brief Inicializa el GPIO del pulsador de pánico con su configuración de entrada/pull-up.
 */
bool PanicButtonInit(void);

/**
 * @brief Lee el estado filtrado (antirrebote) del pulsador.
 * @return true si el botón está presionado.
 */
bool PanicButtonIsPressed(void);

#endif