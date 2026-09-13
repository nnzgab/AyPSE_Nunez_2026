#ifndef CELLULAR_NET_H
#define CELLULAR_NET_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_ALERT_PAYLOAD_SIZE  64

typedef enum {
    CELL_STATE_OFF = 0,
    CELL_STATE_STARTING,
    CELL_STATE_CONNECTING,
    CELL_STATE_READY,
    CELL_STATE_ERROR
} cellular_net_state_t;

typedef enum {
    CELL_NET_OK = 0,
    CELL_NET_ERR_BUSY,
    CELL_NET_ERR_NOT_READY,
    CELL_NET_ERR_TIMEOUT,
    CELL_NET_ERR_PARAM
} cellular_net_err_t;

typedef struct {
    cellular_net_state_t state;
    uint8_t rssi;
    uint8_t consecutive_errors;
    uint32_t uptime_seconds;
} cellular_net_status_t;

/**
 * @brief Inicializa el hardware, crea la Cola de eventos y lanza la tarea de FreeRTOS.
 */
cellular_net_err_t CellularNet_Init(void);

/**
 * @brief Solicita el envío inmediato de una trama. Agrega la alarma a la Cola de FreeRTOS.
 */
cellular_net_err_t CellularNet_SendAlertFrame(const uint8_t *payload, uint16_t length);

/**
 * @brief Estado de salud y métricas.
 */
cellular_net_status_t CellularNet_GetStatus(void);

/**
 * @brief Indica si la red está en READY para transmitir.
 */
bool CellularNet_IsReady(void);

/* 
 * TODO: Funciones pendientes de incorporación a la API pública
 * - cellular_net_err_t CellularNet_DeInit(void); // Liberación de tareas/colas en FreeRTOS
 */

#endif /* CELLULAR_NET_H */