#ifndef CELLULAR_H
#define CELLULAR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define CELLULAR_APN       "datos.personal.com"
#define CELLULAR_USERNAME  "datos"
#define CELLULAR_PASSWORD  "datos"

//#define CELLULAR_TCP_TEST_SERVER "54.175.103.105" //"tcpbin.com"
//#define CELLULAR_TCP_TEST_PORT   30000 //4242

//#define CELLULAR_TCP_TEST_SERVER "tcpbin.com"
//#define CELLULAR_TCP_TEST_PORT   4242


#define CELLULAR_TCP_TEST_SERVER "kvvye-190-183-23-94.run.pinggy-free.link"
#define CELLULAR_TCP_TEST_PORT   40485
//tcp://qextg-190-183-23-94.run.pinggy-free.link:34553       

#define CELLULAR_AT_TIMEOUT_MS        1000   // comandos básicos
#define CELLULAR_QICSGP_TIMEOUT_MS    2000   // configuración PDP
#define CELLULAR_QIACT_TIMEOUT_MS     30000  // activación PDP
#define CELLULAR_QIOPEN_TIMEOUT_MS    60000  // apertura de socket
#define CELLULAR_QICLOSE_TIMEOUT_MS   2000   // cierre de socket




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

bool CellularWaitNetworkRegistration(uint32_t timeout_ms);
bool CellularGetNetworkRegistration(int *status);

bool CellularGetSignalQuality(int *rssi);
bool CellularGetOperator(char *operator_name, size_t operator_size);
bool CellularConfigurePdp(const char *apn,const char *username, const char *password);
bool CellularActivatePdp(void);

/*
return false → no pude ejecutar/interpretar QIACT?
return true  → pude consultar correctamente

active = true  → hay contexto activo
active = false → no hay contexto activo
*/
bool CellularGetPdpStatus( bool *active, char *ip_address, size_t ip_address_size);

/*funcion para comportamiento asincronico*/
bool CellularOpenTcp( int socket_id, const char *server, uint16_t port);
bool CellularCloseTcp(int socket_id);

bool CellularSendCommand( const char *command, char *response, size_t response_size, uint32_t timeout_ms);


bool CellularWaitForResponse(
    const char *expected,
    char *response,
    size_t response_size,
    uint32_t timeout_ms
);

bool CellularPrintSocketState(void);
void CellularCloseAllSockets(void);

int CellularGetOpenSockets(int *sockets, int max_sockets);

bool CellularOpenSocket(
    int socket_id,
    const char *type,   // "TCP" o "UDP"
    const char *host,
    int port
);

/*
Tu función CellularOpenTcp() puede ser simplemente un wrapper que llama a la genérica:
c

bool CellularOpenTcp(int socket_id, const char *server, int port)
{
    return CellularOpenSocket(socket_id, "TCP", server, port);

*/

bool CellularCloseSocket(int socket_id);


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