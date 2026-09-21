#ifndef CELLULAR_NET_H
#define CELLULAR_NET_H

/** @defgroup middleware Middleware
 *  @brief Layer of intermediate logical services.
 *  @{
 *  @defgroup cellular_net Cellular Network Middleware
 *  @brief Service for managing cellular connection, FSM, and alert transmission.
 *  @{
 * 
 * @section genDesc General Description
 * 
 * This middleware manages the finite state machine (FSM) for cellular network connection,
 * PDP context activation, socket management, and FreeRTOS event queueing for alert transmission.
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
#define MAX_ALERT_PAYLOAD_SIZE  64

/*==================[typedef]================================================*/
/**
 * @brief Estados de la máquina de estados finitos (FSM) de red celular.
 */
typedef enum {
    CELL_STATE_OFF = 0,      /**< Módem apagado / inactivo */
    CELL_STATE_STARTING,     /**< Proceso de arranque y verificación AT */
    CELL_STATE_CONNECTING,   /**< Registro en red LTE y activación de PDP */
    CELL_STATE_READY,        /**< Red conectada y lista para transmitir */
    CELL_STATE_ERROR         /**< Estado de error con reintento pasivo */
} cellular_net_state_t;

/**
 * @brief Códigos de error devueltos por la API de red celular.
 */
typedef enum {
    CELL_NET_OK = 0,         /**< Operación exitosa */
    CELL_NET_ERR_BUSY,       /**< Cola de transmisión llena */
    CELL_NET_ERR_NOT_READY,  /**< Módem o red no disponibles para la operación */
    CELL_NET_ERR_TIMEOUT,    /**< Tiempo de espera agotado */
    CELL_NET_ERR_PARAM       /**< Parámetro nulo o inválido */
} cellular_net_err_t;

/**
 * @brief Estructura de estado de salud y métricas de la red celular.
 */
typedef struct {
    cellular_net_state_t state;              /**< Estado actual de la FSM */
    uint8_t              rssi;               /**< Nivel de señal RSSI */
    uint8_t              consecutive_errors; /**< Contador de errores consecutivos */
    uint32_t             uptime_seconds;     /**< Tiempo acumulado en estado READY (segundos) */
} cellular_net_status_t;

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/
/**
 * @brief Inicializa el hardware, crea la Cola de eventos y lanza la tarea de FreeRTOS.
 * 
 * @return cellular_net_err_t CELL_NET_OK si la inicialización fue exitosa.
 */
cellular_net_err_t CellularNet_Init(void);

/**
 * @brief Solicita el envío inmediato de una trama agregando la alarma a la Cola de FreeRTOS.
 * 
 * @param payload Puntero al buffer con los datos a transmitir.
 * @param length Longitud en bytes del payload.
 * @return cellular_net_err_t CELL_NET_OK si se encoló correctamente, CELL_NET_ERR_BUSY si la cola está llena.
 */
cellular_net_err_t CellularNet_SendAlertFrame(const uint8_t *payload, uint16_t length);

/**
 * @brief Obtiene el estado de salud y métricas actuales del módulo de red.
 * 
 * @return cellular_net_status_t Estructura con el estado y métricas.
 */
cellular_net_status_t CellularNet_GetStatus(void);

/**
 * @brief Indica si la red está en estado READY para transmitir datos.
 * 
 * @return true si la red está lista, false en caso contrario.
 */
bool CellularNet_IsReady(void);

/**
 * @brief Obtiene el IMEI real del módem leído de forma segura por la tarea de red.
 * 
 * @param imei_out Buffer donde se copiarán los 15 dígitos ASCII + '\0'.
 * @param max_len Tamaño del buffer (debe ser >= 16).
 * @return true si el IMEI ya fue leído correctamente del hardware, false si usa fallback.
 */
bool CellularNet_GetIMEI(char *imei_out, size_t max_len);

/** @} */
/** @} */

#endif /* #ifndef CELLULAR_NET_H */

/*==================[end of file]============================================*/