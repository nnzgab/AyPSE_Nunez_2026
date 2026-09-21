#ifndef EVENT_FRAME_H
#define EVENT_FRAME_H

/** @defgroup middleware Middleware
 *  @brief Layer of intermediate logical services.
 *  @{
 *  @defgroup event_frame Event Frame Serialization Middleware
 *  @brief Service for packing event data into binary and text frame formats.
 *  @{
 * 
 * @section genDesc General Description
 * 
 * This middleware provides functions to serialize event structures (event_data_t)
 * into binary or CSV-formatted text payloads for network transmission.
 * 
 * @author Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)
 *
 * @section changelog
 *
 * |   Date     | Description                                                            |
 * |:----------:|:----------------------------------------------------------------------|
 * | 20/09/2026 | Document creation and initial implementation                          |
 * 
 **/

/*==================[inclusions]=============================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*==================[macros]=================================================*/
#define EVENT_FRAME_HEADER_MAGIC 0xAA55
#define IMEI_LEN_BYTES           15

/*==================[typedef]================================================*/
/**
 * @brief Identificador único del evento (escalable para sumar más eventos).
 */
typedef enum {
    EVENT_TYPE_PANIC_ALERT = 0x01,  /**< Alerta de pánico */
    EVENT_TYPE_LOW_BATTERY = 0x02,  /**< Batería baja (futuro) */
    EVENT_TYPE_HEARTBEAT   = 0x03   /**< Latido de corazón / Heartbeat (futuro) */
} event_type_t;

/**
 * @brief Códigos de error devueltos por la API de serialización de tramas.
 */
typedef enum {
    EVENT_FRAME_OK = 0,               /**< Operación exitosa */
    EVENT_FRAME_ERR_PARAM,            /**< Parámetro nulo o inválido */
    EVENT_FRAME_ERR_BUFFER_TOO_SMALL  /**< Tamaño de buffer insuficiente */
} event_frame_err_t;

/**
 * @brief Estructura de datos del evento.
 */
typedef struct {
    char         imei[IMEI_LEN_BYTES + 1]; /**< 15 dígitos ASCII + '\0' */
    event_type_t event_type;               /**< Código numérico del evento */
    uint16_t     sequence_number;          /**< Contador incremental */
} event_data_t;

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/
/**
 * @brief Empaqueta una estructura de evento en formato binario.
 * 
 * @param event Puntero a la estructura de datos del evento.
 * @param buffer_out Buffer de salida donde se escribirá la trama binaria.
 * @param buffer_size Tamaño disponible en el buffer de salida.
 * @param packed_len Puntero donde se almacenará la cantidad de bytes escritos.
 * @return event_frame_err_t Código de resultado de la operación.
 */
event_frame_err_t EventFrame_PackBinary(const event_data_t *event, uint8_t *buffer_out, size_t buffer_size, uint16_t *packed_len);

/**
 * @brief Empaqueta una estructura de evento en formato texto CSV.
 * 
 * @param event Puntero a la estructura de datos del evento.
 * @param buffer_out Buffer de texto donde se escribirá la trama formateada.
 * @param buffer_size Tamaño disponible en el buffer de texto.
 * @param packed_len Puntero donde se almacenará la cantidad de caracteres escritos.
 * @return event_frame_err_t Código de resultado de la operación.
 */
event_frame_err_t EventFrame_PackText(const event_data_t *event, char *buffer_out, size_t buffer_size, uint16_t *packed_len);

/** @} */
/** @} */

#endif /* #ifndef EVENT_FRAME_H */

/*==================[end of file]============================================*/