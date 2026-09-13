#include "cellular_modem.h"
#include <string.h>
#include <stdio.h>


#include "gpio_hal.h"
#include "uart_hal.h"
#include "board_config.h"

#define PWRKEY_STABILIZATION_MS  31U
#define PWRKEY_PULSE_TIME_MS     2100U

#define PWRKEY_OFF_TIME_MS       3200U
#define PWRKEY_OFF_PULSE_MS      5000U


/* ============================================================================
 * 1. Inicialización y Control de Alimentación
 * ============================================================================ */

bool CellularModemInit(void)
{
    UartHalInit(UART_BAUDRATE);
    GPIOInit(
        QUECTEL_PWRKEY_PIN,
        GPIO_OUTPUT
    );
    GPIOOff(QUECTEL_PWRKEY_PIN);
    return true;
}

void CellularModemPowerPulse(void)
{
    GPIOOff(QUECTEL_PWRKEY_PIN);
    HalDelayMs(PWRKEY_STABILIZATION_MS);
    GPIOOn(QUECTEL_PWRKEY_PIN);
    HalDelayMs(PWRKEY_PULSE_TIME_MS);
    GPIOOff(QUECTEL_PWRKEY_PIN);
}

bool CellularModemHardPowerOff(void)
{
    GPIOInit(
        QUECTEL_PWRKEY_PIN,
        GPIO_OUTPUT
    );
    GPIOOff(QUECTEL_PWRKEY_PIN);
    HalDelayMs(PWRKEY_OFF_TIME_MS);
    GPIOOn(QUECTEL_PWRKEY_PIN);
    HalDelayMs(PWRKEY_OFF_PULSE_MS);
    GPIOOff(QUECTEL_PWRKEY_PIN);
    return true;
}

bool CellularModemWaitBoot(uint32_t timeout_ms)
{
    char response[128];
    int len = UartHalReadBytes(response, sizeof(response) - 1, timeout_ms);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';
    return (strstr(response, "RDY") != NULL);
}


/* ============================================================================
 * 2. Estado del Módem y Registro de Red
 * ============================================================================ */

bool CellularModemIsReady(void)
{
    char response[64];
    
    // Limpiamos basura en buffer antes de enviar
    UartHalWriteBytes("AT\r\n", 4);
    int len = UartHalReadBytes(response, sizeof(response) - 1, 1000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';
    return (strstr(response, "OK") != NULL);
}

bool CellularModemIsSimReady(void)
{
    char response[128];

    // Consulta el estado de la SIM
    UartHalWriteBytes("AT+CPIN?\r\n", 10);
    int len = UartHalReadBytes(response, sizeof(response) - 1, 3000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    // Debe responder +CPIN: READY y luego OK
    return (strstr(response, "+CPIN: READY") != NULL);
}

bool CellularModemIsNetworkRegistered(void)
{
    char response[128];

    // Consulta el estado de registro en red LTE/EPS
    UartHalWriteBytes("AT+CEREG?\r\n", 11);
    int len = UartHalReadBytes(response, sizeof(response) - 1, 3000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    // +CEREG: <n>,1 -> Registrado en red local
    // +CEREG: <n>,5 -> Registrado en roaming
    bool is_home    = (strstr(response, ",1") != NULL);
    bool is_roaming = (strstr(response, ",5") != NULL);

    return (is_home || is_roaming);
}

/* ============================================================================
 * 3. Identificación del Dispositivo
 * ============================================================================ */

bool CellularModemGetIMEI(char *imei_out, size_t max_len)
{
    if (imei_out == NULL || max_len < 16) {
        return false;
    }

    // CORRECCIÓN 1: Declarar como un buffer de tamaño suficiente
    char response[64]; 
    
    // 1. Enviar el comando AT+CGSN directamente por la UART
    UartHalWriteBytes("AT+CGSN\r\n", 9);
    
    // 2. Leer la respuesta de la UART
    int len = UartHalReadBytes(response, sizeof(response) - 1, 3000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    // 3. Validar que la respuesta contenga "OK"
    if (strstr(response, "OK") == NULL) {
        return false;
    }

    // 4. Buscar el primer dígito del IMEI en la respuesta
    char *start = response;
    while (*start && (*start < '0' || *start > '9')) {
        start++;
    }

    // Verificar que queden al menos 15 dígitos
    if (strlen(start) < 15) {
        return false;
    }

    // Copiar los 15 dígitos del IMEI al buffer de salida
    strncpy(imei_out, start, 15);
    
    // CORRECCIÓN 2: Terminar la cadena en el índice 15 (el IMEI tiene 15 dígitos)
    imei_out[15] = '\0';

    return true;
}

/*========================================================================
 * 4. Configuración y Activación PDP (Contexto de Datos)
 *========================================================================*/

bool CellularModemConfigurePdp(const char *apn, const char *username, const char *password)
{
    if (apn == NULL) {
        return false;
    }

    char cmd[256];
    char response[128];

    const char *user = (username != NULL) ? username : "";
    const char *pass = (password != NULL) ? password : "";
    int auth_type = (strlen(user) > 0) ? 1 : 0; // 1 = PAP, 0 = Sin autenticación

    // Formato: AT+QICSGP=<context_id>,<context_type>,"<apn>","<user>","<pass>",<auth>
    snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",%d\r\n",
             apn, user, pass, auth_type);

    UartHalWriteBytes(cmd, strlen(cmd));
    int len = UartHalReadBytes(response, sizeof(response) - 1, 3000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    return (strstr(response, "OK") != NULL);
}

bool CellularModemActivatePdp(void)
{
    char response[128];

    // Activa el contexto PDP 1
    UartHalWriteBytes("AT+QIACT=1\r\n", 12);

    // Timeout elevado (15s) por la negociación IP con la red
    int len = UartHalReadBytes(response, sizeof(response) - 1, 15000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    return (strstr(response, "OK") != NULL);
}

bool CellularModemIsPdpActive(char *ip_address, size_t ip_address_size)
{
    char response[128];

    // Consulta los contextos PDP activos y sus IP asignadas
    UartHalWriteBytes("AT+QIACT?\r\n", 11);
    int len = UartHalReadBytes(response, sizeof(response) - 1, 3000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    // Verificamos respuesta global OK
    if (strstr(response, "OK") == NULL) {
        return false;
    }

    // Buscamos la línea que indica que el contexto 1 está activo: +QIACT: 1,1,1,"
    char *start = strstr(response, "+QIACT: 1,1,1,\"");
    if (start == NULL) {
        return false; // El contexto PDP 1 no está activo
    }

    // Si el llamador solicita la IP, la extraemos entre comillas
    if (ip_address != NULL && ip_address_size > 0) {
        start += strlen("+QIACT: 1,1,1,\""); // Avanzamos al primer carácter de la IP
        char *end = strchr(start, '"');

        if (end != NULL) {
            size_t ip_len = (size_t)(end - start);
            if (ip_len < ip_address_size) {
                strncpy(ip_address, start, ip_len);
                ip_address[ip_len] = '\0';
            } else {
                return false; // El buffer provisto es muy pequeño
            }
        }
    }

    return true;
}

/* ============================================================================
 * 5. Gestión de Sockets TCP (Operaciones de Red)
 * ============================================================================ */

bool CellularModemSocketOpen(const char *proto, const char *ip, uint16_t port)
{
    if (proto == NULL || ip == NULL || port == 0) {
        return false;
    }

    char cmd[256];
    char response[128];

    // AT+QIOPEN=<contextID>,<connectID>,"<service_type>","<IP>",<port>
    snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,0,\"%s\",\"%s\",%u,0,0\r\n", proto, ip, port);

    UartHalWriteBytes(cmd, strlen(cmd));
    int len = UartHalReadBytes(response, sizeof(response) - 1, 10000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    // Espera OK de aceptación del comando o confirmación de apertura +QIOPEN: 0,0
    return (strstr(response, "OK") != NULL || strstr(response, "+QIOPEN: 0,0") != NULL);
}

bool CellularModemSocketSend(const uint8_t *payload, uint16_t length)
{
    if (payload == NULL || length == 0) {
        return false;
    }

    char cmd[64];
    char response[128];

    // AT+QISEND=<connectID>,<length>
    snprintf(cmd, sizeof(cmd), "AT+QISEND=0,%u\r\n", length);
    UartHalWriteBytes(cmd, strlen(cmd));

    // Esperamos el prompt '>' del módem
    int len = UartHalReadBytes(response, sizeof(response) - 1, 2000);
    if (len <= 0 || strchr(response, '>') == NULL) {
        return false;
    }

    // Enviamos el payload binario
    UartHalWriteBytes((const char *)payload, length);

    // Leemos la confirmación "SEND OK"
    len = UartHalReadBytes(response, sizeof(response) - 1, 5000);
    if (len <= 0) {
        return false;
    }

    response[len] = '\0';
    return (strstr(response, "SEND OK") != NULL);
}

bool CellularModemSocketClose(void)
{
    char response[64];

    // AT+QICLOSE=<connectID>
    UartHalWriteBytes("AT+QICLOSE=0\r\n", 14);
    int len = UartHalReadBytes(response, sizeof(response) - 1, 3000);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';
    return (strstr(response, "OK") != NULL);
}

bool CellularModemSocketReceive(uint8_t *buffer_out, uint16_t max_len, uint16_t *received_len)
{
    if (buffer_out == NULL || max_len == 0) {
        return false;
    }

    // CORRECCIÓN: Declarar como buffers (arreglos)
    char cmd[32];
    char response[512]; // Debe ser mayor a max_len + encabezados AT

    // 1. Solicitar la lectura de bytes del socket 0 vía AT+QIRD
    snprintf(cmd, sizeof(cmd), "AT+QIRD=0,%u\r\n", max_len);
    UartHalWriteBytes(cmd, strlen(cmd));

    // 2. Leer la respuesta devuelta por el módem
    int len = UartHalReadBytes(response, sizeof(response) - 1, 3000);
    if (len <= 0) {
        return false;
    }
    response[len] = '\0';

    // 3. Buscar la respuesta +QIRD: <cantidad>
    char *qird = strstr(response, "+QIRD:");
    if (qird == NULL) {
        return false;
    }

    int count = 0;
    if (sscanf(qird, "+QIRD: %d", &count) != 1 || count <= 0) {
        return false; // Sin datos o error de lectura
    }

    // 4. Ubicar el comienzo del payload tras el primer salto de línea '\n'
    char *payload = strchr(qird, '\n');
    if (payload == NULL) {
        return false;
    }
    payload++; // Saltar el '\n' para apuntar al primer byte del dato

    // 5. Copiar los bytes al buffer de salida
    uint16_t bytes_to_copy = (count < max_len) ? (uint16_t)count : max_len;
    
    // PRECAUCIÓN EXTRA: Validar que no estemos leyendo fuera de nuestro buffer 'response'
    int payload_offset = payload - response;
    if (payload_offset + bytes_to_copy > len) {
        bytes_to_copy = len - payload_offset; // Truncar a lo que realmente se recibió
    }

    memcpy(buffer_out, payload, bytes_to_copy);

    if (received_len != NULL) {
        *received_len = bytes_to_copy;
    }

    return true;
}