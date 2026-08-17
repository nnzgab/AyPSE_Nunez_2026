#ifndef EVENT_FRAME_H
#define EVENT_FRAME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool EventFrameInit(void);

/**
 * @brief Obtiene internamente IMEI y NTP Time, calcula CRC32 y genera la trama.
 * @return true si la trama se construyó correctamente dentro del tamaño disponible.
 */
bool EventFrameBuild(uint8_t status, const char *token, uint8_t *out_buffer, size_t *out_len);

/**
 * @brief Intenta envío vía TCP (con reintentos) y conmuta a SMS como respaldo.
 * @return true si se envió con éxito por alguno de los dos canales.
 */
bool EventFrameDispatch(const uint8_t *buffer, size_t length);

#endif /* EVENT_FRAME_H */