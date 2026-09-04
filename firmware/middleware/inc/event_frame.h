#ifndef EVENT_FRAME_H
#define EVENT_FRAME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Tipos de evento soportados.
 * Alcance actual: solo PANIC. Escalable a futuro con
 * EVENT_TYPE_HEARTBEAT, EVENT_TYPE_LOW_BATTERY, etc.
 */
typedef enum
{
    EVENT_TYPE_PANIC = 0
} event_type_t;

/**
 * @brief Trama de evento.
 * timestamp y crc quedan reservados: no se completan ni se envían
 * en el alcance actual de la materia.
 */
typedef struct
{
    event_type_t type;
    char imei[16];
    uint32_t timestamp; /* Reservado para futura implementación */
    uint16_t crc;        /* Reservado para futura implementación */
} event_frame_t;

/**
 * @brief Construye la trama en texto plano: "<TIPO>,<IMEI>"
 *        (ej: "PANIC,864920040123456")
 * @return Cantidad de bytes escritos (sin '\0'), o -1 si no entra en buffer_out.
 */
int EventFrameBuild(const event_frame_t *frame, char *buffer_out, size_t max_len);



#endif /* EVENT_FRAME_H */