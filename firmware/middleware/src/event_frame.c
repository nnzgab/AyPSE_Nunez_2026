#include "event_frame.h"
#include <stdio.h>
#include <string.h>

event_frame_err_t EventFrame_PackBinary(const event_data_t *event, 
                                       uint8_t *buffer_out, 
                                       size_t buffer_size, 
                                       uint16_t *packed_len) {
    if (event == NULL || buffer_out == NULL || packed_len == NULL) {
        return EVENT_FRAME_ERR_PARAM;
    }

    const size_t total_payload_size = 20;

    if (buffer_size < total_payload_size) {
        return EVENT_FRAME_ERR_BUFFER_TOO_SMALL;
    }

    size_t idx = 0;

    // Header Magic (0xAA55)
    buffer_out[idx++] = (uint8_t)(EVENT_FRAME_HEADER_MAGIC >> 8);
    buffer_out[idx++] = (uint8_t)(EVENT_FRAME_HEADER_MAGIC & 0xFF);

    // Tipo de Evento numérico (0x01)
    buffer_out[idx++] = (uint8_t)event->event_type;

    // IMEI (15 bytes ASCII)
    memcpy(&buffer_out[idx], event->imei, IMEI_LEN_BYTES);
    idx += IMEI_LEN_BYTES;

    // Número de Secuencia (2 bytes - Big Endian)
    buffer_out[idx++] = (uint8_t)(event->sequence_number >> 8);
    buffer_out[idx++] = (uint8_t)(event->sequence_number & 0xFF);

    *packed_len = (uint16_t)idx;
    return EVENT_FRAME_OK;
}

event_frame_err_t EventFrame_PackText(const event_data_t *event, 
                                     char *buffer_out, 
                                     size_t buffer_size, 
                                     uint16_t *packed_len) {
    if (event == NULL || buffer_out == NULL || packed_len == NULL) {
        return EVENT_FRAME_ERR_PARAM;
    }

    /* Formato CSV numérico: TIPO_EVENTO_HEX,IMEI,SECUENCIA */
    int written = snprintf(buffer_out, buffer_size,
                           "%02X,%s,%u\r\n",
                           (unsigned int)event->event_type, // Muestra "01"
                           event->imei,
                           (unsigned int)event->sequence_number);

    if (written < 0 || (size_t)written >= buffer_size) {
        return EVENT_FRAME_ERR_BUFFER_TOO_SMALL;
    }

    *packed_len = (uint16_t)written;
    return EVENT_FRAME_OK;
}