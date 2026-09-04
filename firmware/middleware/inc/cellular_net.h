#ifndef CELLULAR_NET_H
#define CELLULAR_NET_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

/* ============================================================
 * Estados de la red celular
 * ============================================================ */

typedef enum
{
    NET_STATUS_DISCONNECTED = 0,
    NET_STATUS_REGISTERING,
    NET_STATUS_READY,
    NET_STATUS_SOCKET_CONNECTED
} cellular_net_status_t;

/* ============================================================
 * Inicialización
 * ============================================================ */

/**
 * @brief Inicializa el módem para utilizar la red celular.
 *
 * No configura ni activa todavía el PDP.
 */
esp_err_t CellularNetInit(void);

/* ============================================================
 * Conexión a la red
 * ============================================================ */

/**
 * @brief Configura y activa el contexto PDP.
 *
 * @param apn APN del operador celular.
 */
esp_err_t CellularNetConnectApn(const char *apn, const char *user, const char *password);

/* ============================================================
 * TCP
 * ============================================================ */

/**
 * @brief Abre una conexión TCP.
 *
 * @param ip Dirección IP o servidor remoto.
 * @param port Puerto TCP.
 */
esp_err_t CellularNetOpenTcp(
    const char *ip,
    uint16_t port
);

/**
 * @brief Envía datos por la conexión TCP.
 */
esp_err_t CellularNetSendData(
    const uint8_t *data,
    size_t length
);

/**
 * @brief Recibe datos de la conexión TCP.
 */
esp_err_t CellularNetReceiveData(
    uint8_t *data,
    size_t data_size,
    size_t *received
);

/**
 * @brief Cierra la conexión TCP.
 */
esp_err_t CellularNetCloseTcp(void);

/* ============================================================
 * Estado
 * ============================================================ */

cellular_net_status_t CellularNetGetStatus(void);

#endif /* CELLULAR_NET_H */