#ifndef PANIC_BUTTON_H
#define PANIC_BUTTON_H

#include <stdbool.h>

/**
 * @brief Inicializa el botón de pánico.
 *
 * Configura el GPIO como entrada y activa
 * la interrupción correspondiente.
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
 * @brief Consulta si ocurrió una pulsación.
 *
 * El evento se consume al ser leído.
 *
 * @return true si hubo una pulsación.
 */
bool PanicButtonPressedEvent(void);

#endif  /* PANIC_BUTTON_H */