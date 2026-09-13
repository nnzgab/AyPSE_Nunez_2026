#ifndef CELLULAR_MODEM_H
#define CELLULAR_MODEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * 1. Inicialización y Control Físico de Energía
 * ============================================================================ */

/**
 * @brief Configura periféricos (UART, GPIOs de PWRKEY, RESET, etc.).
 */
bool CellularModemInit(void);//

/**
 * @brief Envía un pulso físico al pin PWRKEY para encender/encender suavemente.
 */
void CellularModemPowerPulse(void);///

/**
 * @brief Ejecuta un apagado forzado por hardware (corte o pin de Reset/Power).
 */
bool CellularModemHardPowerOff(void);///


/* ============================================================================
 * Arranque y Verificación de UART
 * ============================================================================ */

/**
 * @brief Escucha la UART esperando el URC "RDY" tras el encendido.
 * @note Solo debe llamarse una vez inmediatamente despues del PWRKEY.
 */
bool CellularModemWaitBoot(uint32_t timeout_ms);

/**
 * @brief Envia 'AT\r\n' y verifica si el modem responde 'OK'.
 */
bool CellularModemIsReady(void);
/* ============================================================================
 * 2. Verificaciones de Estado (IsReady, SIM, Red)
 * ============================================================================ */


/**
 * @brief Consulta estado del chip (AT+CPIN?).
 * @return true si devuelve +CPIN: READY.
 */
bool CellularModemIsSimReady(void);

/**
 * @brief Obtiene la calidad de señal RSSI (AT+CSQ).
 * @param[out] rssi Puntero donde se guarda el valor (0-31, o 99 si no hay señal).
 */
bool CellularModemGetSignalQuality(uint8_t *rssi);

/**
 * @brief Consulta el registro en la red celular (AT+CEREG?).
 * @return true si está registrado (estado 1=Local o 5=Roaming).
 */
bool CellularModemIsNetworkRegistered(void);

/**
 * @brief Obtiene el número IMEI del módulo (AT+CGSN / AT+GSN).
 */
bool CellularModemGetImei(char *imei_buffer, size_t buffer_size);

/* ============================================================================
 * 4. Configuración y Activación de Datos (PDP / IP)
 * ============================================================================ */

/**
 * @brief Configura el contexto PDP con el APN y credenciales (AT+QICSGP).
 */
bool CellularModemConfigurePdp(
    const char *apn,
    const char *username,
    const char *password
);

/**
 * @brief Activa la conexión de datos de la operadora (AT+QIACT=1).
 * @note Puede demorar hasta 15 segundos en negociar.
 */
bool CellularModemActivatePdp(void);

/**
 * @brief Verifica si el PDP está activo y obtiene la IP asignada (AT+QIACT?).
 * @param[out] ip_address Buffer opcional para almacenar la IP obtenida.
 * @param[in] ip_address_size Tamaño del buffer de IP.
 */
bool CellularModemIsPdpActive(char *ip_address, size_t ip_address_size);


/* ============================================================================
 * 5. Sockets y Comunicaciones TCP
 * ============================================================================ */

/**
 * @brief Abre una conexión TCP/UDP hacia un servidor y puerto.
 * @param proto Protocolo de capa de transporte (ej: "TCP" o "UDP").
 * @param ip Dirección IP de destino en formato string (ej: "203.0.113.10").
 * @param port Puerto de destino.
 * @return true si la conexión se abrió con éxito.
 */
bool CellularModemSocketOpen(const char *proto, const char *ip, uint16_t port);

/**
 * @brief Envía un buffer de bytes binarios por el socket abierto.
 * @param payload Puntero al buffer de datos a transmitir.
 * @param length Cantidad de bytes a enviar.
 * @return true si el módem respondió confirmation de envío (SEND OK).
 */
bool CellularModemSocketSend(const uint8_t *payload, uint16_t length);

/**
 * @brief Cierra el socket activo en el módem (ej: AT+QICLOSE).
 * @return true si el socket se cerró correctamente.
 */
bool CellularModemSocketClose(void);

bool CellularModemGetIMEI(char *imei_out, size_t max_len);


bool CellularModemSocketReceive(uint8_t *buffer_out, uint16_t max_len, uint16_t *received_len);


#endif /* CELLULAR_MODEM_H */