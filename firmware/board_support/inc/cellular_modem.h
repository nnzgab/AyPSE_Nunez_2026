#ifndef CELLULAR_MODEM_H
#define CELLULAR_MODEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ============================================================
 * Inicialización
 * ============================================================ */

bool CellularModemInit(void);



/* ============================================================
 * Estado
 * ============================================================ */

bool CellularModemIsReady(void);

bool CellularModemWaitReady(uint32_t timeout_ms);

/* ============================================================
 * Comandos AT
 * ============================================================ */

bool CellularModemSendCommand(
    const char *command,
    char *response,
    size_t response_size,
    uint32_t timeout_ms
);

/* ============================================================
 * Identificación
 * ============================================================ */

bool CellularModemGetImei(
    char *imei,
    size_t imei_size
);

/* ============================================================
 * Red celular / PDP
 * ============================================================ */

bool CellularModemConfigurePdp(
    const char *apn,
    const char *username,
    const char *password
);

bool CellularModemActivatePdp(void);

bool CellularModemIsPdpActive(void);

/* ============================================================
 * TCP
 * ============================================================ */

bool CellularModemOpenTcp(
    const char *server,
    uint16_t port
);

bool CellularModemSendTcp(
    const uint8_t *data,
    size_t length
);

bool CellularModemReceiveTcp(
    uint8_t *data,
    size_t data_size,
    size_t *received
);

bool CellularModemCloseTcp(void);

#endif /* CELLULAR_MODEM_H */