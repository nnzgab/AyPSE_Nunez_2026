#ifndef EVENT_FRAME_H
#define EVENT_FRAME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define EVENT_FRAME_HEADER_MAGIC 0xAA55
#define IMEI_LEN_BYTES           15

/**
 * @brief Identificador único del evento (escalable para sumar más eventos)
 */
typedef enum {
    EVENT_TYPE_PANIC_ALERT = 0x01,
    EVENT_TYPE_LOW_BATTERY = 0x02, // Futuro
    EVENT_TYPE_HEARTBEAT   = 0x03  // Futuro
} event_type_t;

typedef enum {
    EVENT_FRAME_OK = 0,
    EVENT_FRAME_ERR_PARAM,
    EVENT_FRAME_ERR_BUFFER_TOO_SMALL
} event_frame_err_t;

/**
 * @brief Estructura de la trama
 */
typedef struct {
    char         imei[IMEI_LEN_BYTES + 1]; // 15 dígitos ASCII + '\0'
    event_type_t event_type;               // Código numérico (ej: EVENT_TYPE_PANIC_ALERT = 0x01)
    uint16_t     sequence_number;          // Contador incremental
} event_data_t;

/* API de Serialización */
event_frame_err_t EventFrame_PackBinary(const event_data_t *event, 
                                       uint8_t *buffer_out, 
                                       size_t buffer_size, 
                                       uint16_t *packed_len);

event_frame_err_t EventFrame_PackText(const event_data_t *event, 
                                     char *buffer_out, 
                                     size_t buffer_size, 
                                     uint16_t *packed_len);

#endif /* EVENT_FRAME_H */