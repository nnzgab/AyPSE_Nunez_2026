#ifndef CELLULAR_H
#define CELLULAR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


/* ================== Configuración ================== */

#define CELLULAR_APN       "datos.personal.com"
#define CELLULAR_USERNAME  "datos"
#define CELLULAR_PASSWORD  "datos"

//#define CELLULAR_TCP_TEST_SERVER "54.175.103.105" //"tcpbin.com"
//#define CELLULAR_TCP_TEST_PORT   30000 //4242

//#define CELLULAR_TCP_TEST_SERVER "tcpbin.com"
//#define CELLULAR_TCP_TEST_PORT   4242
//nsjfz-190-183-23-94.run.pinggy-free.link:38577

#define CELLULAR_TCP_TEST_SERVER "kkfdu-190-183-23-94.run.pinggy-free.link"
#define CELLULAR_TCP_TEST_PORT   33291
   

/*==================[external functions declaration]=========================*/

/* ================== Inicialización y estado ================== */

bool CellularPowerOn(void);
bool CellularWaitReady(uint32_t timeout_ms);
bool CellularIsReady(void);
bool CellularPowerOff(void);
bool CellularPowerOffHard(void);


/* ================== Configuración básica ================== */

bool CellularEchoOff(void);
bool CellularSetFullFunction(void);
bool CellularGetIMSI(char *imsi, size_t imsi_size);


/* ================== Registro en red ================== */

bool CellularWaitNetworkRegistration(uint32_t timeout_ms);
bool CellularGetNetworkRegistration(int *status);
bool CellularGetSignalQuality(int *rssi);
bool CellularGetOperator(char *operator_name, size_t operator_size);


/* ================== PDP (Packet Data Protocol) ================== */

bool CellularConfigurePdp(const char *apn,const char *username, const char *password);
bool CellularActivatePdp(void);
/*
return false → no pude ejecutar/interpretar QIACT?
return true  → pude consultar correctamente

active = true  → hay contexto activo
active = false → no hay contexto activo
*/
bool CellularGetPdpStatus( bool *active, char *ip_address, size_t ip_address_size);


/* ================== Utilidades AT ================== */

bool CellularSendCommand( const char *command, char *response, size_t response_size, uint32_t timeout_ms);
bool CellularWaitForResponse(const char *expected, char *response, size_t response_size, uint32_t timeout_ms);


/* ================== Manejo de sockets ================== */

bool CellularPrintSocketState(void);
void CellularCloseAllSockets(void);
int CellularGetOpenSockets(int *sockets, int max_sockets);

// type"TCP" o "UDP"
bool CellularOpenSocket(int socket_id, const char *type, const char *host, int port);
bool CellularCloseSocket(int socket_id);

bool CellularSendTcp(int socket_id, const char *data, size_t length);
bool CellularReceiveTcp(int socket_id, char *data, size_t data_size);


// TODO: implementar en el futuro
 bool CellularGetImei(char *imei_out, size_t max_len);
 bool CellularGetNtpTime(char *timestamp_out, size_t max_len);
 bool CellularSmsSend(const char *number, const char *message);


#endif /* CELLULAR_H */