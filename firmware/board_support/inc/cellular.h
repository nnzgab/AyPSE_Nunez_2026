#ifndef CELLULAR_H
#define CELLULAR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Servicios de Control y Red definidos para el módulo celular[cite: 2] */
bool CellularInit(void);
bool CellularReset(void);
bool CellularIsReady(void);
bool CellularConnect(void);
bool CellularDisconnect(void);
bool CellularPdpConfigure(const char *apn);
bool CellularPdpActivate(void);

/* Consultas requeridas por el Middleware (IMEI y NTP)[cite: 1] */
bool CellularGetImei(char *imei_out, size_t max_len);
bool CellularGetNtpTime(char *timestamp_out, size_t max_len);

/* Manejo de Sockets TCP y SMS[cite: 2] */
bool CellularTcpConnect(const char *host, uint16_t port);
bool CellularTcpSend(const uint8_t *buffer, size_t length);
int CellularTcpReceive(uint8_t *buffer, size_t length, uint32_t timeout_ms);
bool CellularTcpDisconnect(void);
bool CellularSmsSend(const char *number, const char *message);

#endif /* CELLULAR_H */