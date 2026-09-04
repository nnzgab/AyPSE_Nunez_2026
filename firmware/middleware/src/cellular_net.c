#include "cellular_net.h"
#include "cellular_modem.h"

/* ============================================================
 * Estado interno
 * ============================================================ */

static cellular_net_status_t net_status = NET_STATUS_DISCONNECTED;

/* ============================================================
 * Inicialización
 * ============================================================ */

esp_err_t CellularNetInit(void)
{
    /*
     * La inicialización del hardware y el encendido
     * del módem pertenecen a cellular_modem. Inicializa el hardware y espera el RDY del arranque.
     */
    if (!CellularModemInit())
    {
        net_status = NET_STATUS_DISCONNECTED;
        return ESP_FAIL;
    }
    /*
     * 2. Verificación activa: le manda un "AT" al módem 
     * para confirmar que responde y está listo para operar.
     */
    if (!CellularModemIsReady())
        {
            net_status = NET_STATUS_DISCONNECTED;
            return ESP_ERR_TIMEOUT;
        }

        net_status = NET_STATUS_DISCONNECTED;

        return ESP_OK;
    }



/* ============================================================
 * Conexión PDP
 * ============================================================ */


esp_err_t CellularNetConnectApn(
    const char *apn,
    const char *username,
    const char *password
);

esp_err_t CellularNetConnectApn(
    const char *apn,
    const char *username,
    const char *password
)
{
    if (apn == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Configuramos el APN.
     *
     * cellular_modem se encarga de traducir esto
     * a los comandos AT correspondientes.
     */
    if (!CellularModemConfigurePdp(
            apn,
            username,
            password))
    {
        net_status = NET_STATUS_DISCONNECTED;
        return ESP_FAIL;
    }

    net_status = NET_STATUS_REGISTERING;

    /*
     * Activamos el contexto PDP.
     */
    if (!CellularModemActivatePdp())
    {
        net_status = NET_STATUS_DISCONNECTED;
        return ESP_FAIL;
    }

    /*
     * Verificamos que realmente haya quedado activo.
     */
    if (!CellularModemIsPdpActive())
    {
        net_status = NET_STATUS_DISCONNECTED;
        return ESP_FAIL;
    }

    net_status = NET_STATUS_READY;

    return ESP_OK;
}


/* ============================================================
 * Apertura TCP
 * ============================================================ */

esp_err_t CellularNetOpenTcp(
    const char *ip,
    uint16_t port
)
{
    if (ip == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Para abrir TCP primero necesitamos tener
     * la red celular disponible.
     */
    if (net_status != NET_STATUS_READY)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!CellularModemOpenTcp(ip, port))
    {
        net_status = NET_STATUS_READY;
        return ESP_FAIL;
    }

    net_status = NET_STATUS_SOCKET_CONNECTED;

    return ESP_OK;
}

/* ============================================================
 * Envío TCP
 * ============================================================ */

esp_err_t CellularNetSendData(
    const uint8_t *data,
    size_t length
)
{
    if (data == NULL || length == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (net_status != NET_STATUS_SOCKET_CONNECTED)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!CellularModemSendTcp(data, length))
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* ============================================================
 * Recepción TCP
 * ============================================================ */

esp_err_t CellularNetReceiveData(
    uint8_t *data,
    size_t data_size,
    size_t *received
)
{
    if (data == NULL || received == NULL || data_size == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (net_status != NET_STATUS_SOCKET_CONNECTED)
    {
        return ESP_ERR_INVALID_STATE;
    }

    *received = 0U;

    if (!CellularModemReceiveTcp(
            data,
            data_size,
            received))
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* ============================================================
 * Cierre TCP
 * ============================================================ */

esp_err_t CellularNetCloseTcp(void)
{
    if (net_status != NET_STATUS_SOCKET_CONNECTED)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!CellularModemCloseTcp())
    {
        return ESP_FAIL;
    }

    net_status = NET_STATUS_READY;

    return ESP_OK;
}

/* ============================================================
 * Estado
 * ============================================================ */

cellular_net_status_t CellularNetGetStatus(void)
{
    return net_status;
}