#ifndef CELLULAR_H
#define CELLULAR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


/*==================[external functions declaration]=========================*/

/* Inicialización y estado */
/* Servicios de Control y Red definidos para el módulo celular*/

bool CellularInit(void);
bool cellularPowerOn(void);
bool CellularReset(void);

bool CellularPowerOn(void);

bool CellularWaitReady(uint32_t timeout_ms);


bool CellularEchoOff(void);
bool CellularSetFullFunction(void);
bool CellularGetIMSI(char *imsi, size_t imsi_size);



bool CellularIsReady(void);


bool CellularPowerOff(void);
bool CellularPowerOffHard(void);


/* Conexión de red */

bool CellularConnect(void);

bool CellularDisconnect(void);


/* PDP */

bool CellularPdpConfigure(const char *apn);

bool CellularPdpActivate(void);

/* Consultas requeridas por el Middleware (IMEI y NTP) */
bool CellularGetImei(char *imei_out, size_t max_len);
bool CellularGetNtpTime(char *timestamp_out, size_t max_len);

/* Manejo de Sockets TCP y SMS */
bool CellularTcpConnect(const char *host, uint16_t port);
bool CellularTcpSend(const uint8_t *buffer, size_t length);
int CellularTcpReceive(uint8_t *buffer, size_t length, uint32_t timeout_ms);
bool CellularTcpDisconnect(void);

/* SMS */
bool CellularSmsSend(const char *number, const char *message);

#endif /* CELLULAR_H */