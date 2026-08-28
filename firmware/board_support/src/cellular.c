
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "cellular.h"
#include "uart_hal.h"
#include "board_config.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Configuración interna
 * ============================================================ */


#define CELLULAR_PWRKEY_STABILIZATION_MS 31
#define CELLULAR_PWRKEY_PULSE_ON_TIME_MS 2100
#define CELLULAR_PWRKEY_ON_TIME_MS       100
#define CELLULAR_PWRKEY_OFF_TIME_MS      2100
#define CELLULAR_BOOT_WAIT_MS            5000

#define CELLULAR_AT_TIMEOUT_MS           1000
#define CELLULAR_POWEROFF_TIMEOUT_MS     5000
#define CELLULAR_CFUN_TIMEOUT_MS    10000

#define CELLULAR_NETWORK_POLL_MS       1000
#define CELLULAR_NETWORK_TIMEOUT_MS    120000
#define CELLULAR_QIACT_TIMEOUT_MS    160000

#define CELLULAR_QIOPEN_COMMAND_TIMEOUT_MS    5000
#define CELLULAR_QIOPEN_RESULT_TIMEOUT_MS    30000




bool CellularPowerOn(void)
{
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);

    // Asegurar línea liberada (no presionado) antes de iniciar
    GPIOOff(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(CELLULAR_PWRKEY_STABILIZATION_MS)); // >= 30 ms

    // Presionar PWRKEY (HIGH -> transistor satura -> PWRKEY a GND)
    GPIOOn(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(CELLULAR_PWRKEY_PULSE_ON_TIME_MS)); // >= 2000 ms

    // Soltar PWRKEY (LOW -> transistor corta -> PWRKEY flota a alto vía pull-up)
    GPIOOff(QUECTEL_PWRKEY_PIN);

    return true;
}


bool CellularWaitReady(uint32_t timeout_ms)/************* */
{
    char response[256];

    int len = UartHalReadBytes(response, sizeof(response) - 1, timeout_ms);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    printf("CELLULAR RX: %s\n", response);

    return strstr(response, "RDY") != NULL;
}


bool CellularIsReady(void)
{
    const char *command = "AT\r\n";

    int written = UartHalWriteBytes(command, strlen(command));
    if (written <= 0) {
        printf("CELLULAR AT TX failed\n");
        return false;
    }

    char response[256];
    int len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        CELLULAR_AT_TIMEOUT_MS
    );

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';
    printf("CELLULAR AT RX: %s\n", response);

    return strstr(response, "OK") != NULL;
}


/* ============================================================
 * Envío de comandos AT
 * ============================================================ */


bool CellularSendCommand(
    const char *command,
    char *response,
    size_t response_size,
    uint32_t timeout_ms
)
{
    if (command == NULL ||
        response == NULL ||
        response_size == 0)
    {
        return false;
    }

    /*
     * Limpiamos el buffer antes de utilizarlo.
     */
    response[0] = '\0';

    /*
     * Enviamos el comando al EG915U.
     */
    int written = UartHalWriteBytes(command, strlen(command));

    if (written <= 0)
    {
        printf("CELLULAR TX failed: %s", command);
        return false;
    }

    printf("CELLULAR TX: %s", command);

    /*
     * Esperamos la respuesta.
     */
    int len = UartHalReadBytes(
        response,
        response_size - 1,
        timeout_ms
    );

    if (len <= 0)
    {
        printf("CELLULAR RX timeout\n");
        return false;
    }

    /*
     * Terminamos la cadena.
     */
    response[len] = '\0';

    printf("CELLULAR RX: %s\n", response);

    return true;
}



bool CellularEchoOff(void)
{
    char response[128];

    if (!CellularSendCommand(
            "ATE0\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    return strstr(response, "OK") != NULL;
}

bool CellularSetFullFunction(void)
{
    char response[128];

    if (!CellularSendCommand(
            "AT+CFUN=1\r\n",
            response,
            sizeof(response),
            CELLULAR_CFUN_TIMEOUT_MS))
    {
        return false;
    }

    return strstr(response, "OK") != NULL;
}


bool CellularGetIMSI(char *imsi, size_t imsi_size)
{
    char response[128];

    if (imsi == NULL || imsi_size == 0)
    {
        return false;
    }

    if (!CellularSendCommand(
            "AT+CIMI\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    /*
     * Buscamos el IMSI dentro de la respuesta.
     */
    char *start = response;

    while (*start == '\r' || *start == '\n')
    {
        start++;
    }

    /*
     * Copiamos hasta CR/LF.
     */
    size_t i = 0;

    while (start[i] != '\0' &&
           start[i] != '\r' &&
           start[i] != '\n' &&
           i < imsi_size - 1)
    {
        imsi[i] = start[i];
        i++;
    }

    imsi[i] = '\0';

    /*
     * Un IMSI normalmente tiene 15 dígitos.
     */
    if (i != 15)
    {
        printf("CELLULAR: Invalid IMSI length: %d\n", (int)i);
        return false;
    }

    /*
     * Verificamos que todos sean dígitos.
     */
    for (i = 0; i < 15; i++)
    {
        if (imsi[i] < '0' || imsi[i] > '9')
        {
            printf("CELLULAR: Invalid IMSI\n");
            return false;
        }
    }

    printf("CELLULAR IMSI: %s\n", imsi);

    return true;
}

bool CellularWaitNetworkRegistration(uint32_t timeout_ms)
{
    uint32_t elapsed_ms = 0;

    while (elapsed_ms < timeout_ms)
    {
        int status;

        if (!CellularGetNetworkRegistration(&status))
        {
            printf("CELLULAR: Failed to get network registration\n");
        }
        else
        {
            printf(
                "CELLULAR: Registration status = %d\n",
                status
            );

            /*
             * 1 = registrado en red local
             * 5 = registrado en roaming
             */
            if (status == 1 || status == 5)
            {
                printf("CELLULAR: Network registered!\n");
                return true;
            }

            /*
             * 3 = registro rechazado.
             *
             * No tiene sentido seguir esperando
             * indefinidamente en este caso.
             */
            if (status == 3)
            {
                printf("CELLULAR: Network registration rejected\n");
                return false;
            }
        }

        vTaskDelay(
            pdMS_TO_TICKS(CELLULAR_NETWORK_POLL_MS)
        );

        elapsed_ms += CELLULAR_NETWORK_POLL_MS;
    }

    printf("CELLULAR: Network registration timeout\n");

    return false;
}


bool CellularGetNetworkRegistration(int *status)
{
    char response[128];

    if (status == NULL)
    {
        return false;
    }

    if (!CellularSendCommand(
            "AT+CEREG?\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    /*
     * Buscamos la respuesta +CEREG.
     *
     * Ejemplo:
     * +CEREG: 0,2
     * OK
     */
    char *cereg = strstr(response, "+CEREG:");

    if (cereg == NULL)
    {
        printf("CELLULAR: CEREG response not found\n");
        return false;
    }

    /*
     * Buscamos la coma.
     *
     * +CEREG: 0,2
     *          ^
     */
    char *comma = strchr(cereg, ',');

    if (comma == NULL)
    {
        printf("CELLULAR: Invalid CEREG response\n");
        return false;
    }

    /*
     * El estado está después de la coma.
     *
     * +CEREG: 0,2
     *            ^
     */
    int registration_status;

    if (sscanf(comma + 1, "%d", &registration_status) != 1)
    {
        printf("CELLULAR: Cannot parse CEREG status\n");
        return false;
    }

    *status = registration_status;

    printf(
        "CELLULAR: Network registration status = %d\n",
        *status
    );

    return true;
}


bool CellularGetSignalQuality(int *rssi)
{
    char response[128];

    if (rssi == NULL)
    {
        return false;
    }

    if (!CellularSendCommand(
            "AT+CSQ\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    /*
     * Ejemplo:
     *
     * +CSQ: 18,99
     * OK
     */

    char *csq = strstr(response, "+CSQ:");

    if (csq == NULL)
    {
        printf("CELLULAR: CSQ response not found\n");
        return false;
    }

    char *colon = strchr(csq, ':');

    if (colon == NULL)
    {
        printf("CELLULAR: Invalid CSQ response\n");
        return false;
    }

    int signal;
    int ber;

    if (sscanf(colon + 1, "%d,%d", &signal, &ber) != 2)
    {
        printf("CELLULAR: Cannot parse CSQ response\n");
        return false;
    }

    *rssi = signal;

    printf(
        "CELLULAR: Signal RSSI = %d, BER = %d\n",
        signal,
        ber
    );

    return true;
}


bool CellularGetOperator(
    char *operator_name,
    size_t operator_size
)
{
    char response[256];

    if (operator_name == NULL || operator_size == 0)
    {
        return false;
    }

    operator_name[0] = '\0';

    if (!CellularSendCommand(
            "AT+COPS?\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    /*
     * Ejemplo:
     *
     * +COPS: 0,0,"Personal",7
     * OK
     */

    char *cops = strstr(response, "+COPS:");

    if (cops == NULL)
    {
        printf("CELLULAR: COPS response not found\n");
        return false;
    }

    char *first_quote = strchr(cops, '"');

    if (first_quote == NULL)
    {
        printf("CELLULAR: Operator name not found\n");
        return false;
    }

    first_quote++;

    char *second_quote = strchr(first_quote, '"');

    if (second_quote == NULL)
    {
        printf("CELLULAR: Invalid COPS response\n");
        return false;
    }

    size_t length = second_quote - first_quote;

    if (length >= operator_size)
    {
        printf("CELLULAR: Operator name buffer too small\n");
        return false;
    }

    memcpy(operator_name, first_quote, length);

    operator_name[length] = '\0';

    printf(
        "CELLULAR: Operator = %s\n",
        operator_name
    );

    return true;
}


bool CellularConfigurePdp(
    const char *apn,
    const char *username,
    const char *password
)
{
    char command[256];
    char response[128];

    if (apn == NULL ||
        username == NULL ||
        password == NULL)
    {
        return false;
    }

    snprintf(
        command,
        sizeof(command),
        "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n",
        apn,
        username,
        password
    );

    if (!CellularSendCommand(
            command,
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    if (strstr(response, "OK") == NULL)
    {
        printf("CELLULAR: QICSGP failed\n");
        return false;
    }

    printf("CELLULAR: PDP context configured\n");

    return true;
}



bool CellularActivatePdp(void)
{
    char response[128];

    if (!CellularSendCommand(
            "AT+QIACT=1\r\n",
            response,
            sizeof(response),
            CELLULAR_QIACT_TIMEOUT_MS))
    {
        printf("CELLULAR: QIACT command failed\n");
        return false;
    }

    if (strstr(response, "OK") == NULL)
    {
        printf("CELLULAR: PDP activation failed\n");
        return false;
    }

    printf("CELLULAR: PDP context activated\n");

    return true;
}



bool CellularGetPdpStatus( bool *active, char *ip_address, size_t ip_address_size)
{
    char response[256];

    if (active == NULL ||
        ip_address == NULL ||
        ip_address_size == 0)
    {
        return false;
    }

    *active = false;
    ip_address[0] = '\0';

    if (!CellularSendCommand(
            "AT+QIACT?\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        printf("CELLULAR: QIACT? command failed\n");
        return false;
    }

    /*
     * Ejemplo:
     *
     * +QIACT: 1,1,1,"10.123.45.67"
     * OK
     */

    char *qiact = strstr(response, "+QIACT:");

    if (qiact == NULL)
    {
        printf("CELLULAR: QIACT response not found\n");
        return false;
    }

    int context_id;
    int context_state;
    int context_type;

    char ip[64];

    int parsed = sscanf(
        qiact,
        "+QIACT: %d,%d,%d,\"%63[^\"]\"",
        &context_id,
        &context_state,
        &context_type,
        ip
    );

    if (parsed != 4)
    {
        printf("CELLULAR: Cannot parse QIACT response\n");
        return false;
    }

    printf(
        "CELLULAR: PDP ID=%d STATE=%d TYPE=%d IP=%s\n",
        context_id,
        context_state,
        context_type,
        ip
    );

    /*
     * Nos interesa específicamente el contexto 1.
     */
    if (context_id != 1)
    {
        printf("CELLULAR: Unexpected PDP context ID\n");
        return false;
    }

    /*
     * context_state == 1 significa activo.
     */
    if (context_state == 1)
    {
        *active = true;
    }

    /*
     * Copiamos la IP al buffer del usuario.
     */
    if (strlen(ip) >= ip_address_size)
    {
        printf("CELLULAR: IP buffer too small\n");
        return false;
    }

    strcpy(ip_address, ip);

    return true;
}




bool CellularOpenTcp(
    int socket_id,
    const char *server,
    uint16_t port
)
{
    char command[256];
    char response[128];

    if (server == NULL)
    {
        return false;
    }

    snprintf(
        command,
        sizeof(command),
        "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%u,0,0\r\n",
        socket_id,
        server,
        port
    );

    printf(
        "CELLULAR: Opening TCP connection to %s:%u\n",
        server,
        port
    );
    /*
     * --------------------------------------------------------
     * ETAPA 1:
     * Enviar QIOPEN y esperar respuesta inmediata.
     * --------------------------------------------------------
     */
    /*
     * Primero esperamos la respuesta inmediata.
     */
    if (!CellularSendCommand(
            command,
            response,
            sizeof(response),
            CELLULAR_QIOPEN_COMMAND_TIMEOUT_MS))
    {
        printf("CELLULAR: QIOPEN command failed\n");
        return false;
    }

    /*
     * QIOPEN debe devolver OK inmediatamente.
     */
    if (strstr(response, "OK") == NULL)
    {
        printf("CELLULAR: QIOPEN rejected\n");
        return false;
    }

    char expected[64];
    snprintf(expected, sizeof(expected), "+QIOPEN: %d,0", socket_id);

    /*
     * Puede que la URC ya haya llegado pegada al OK, dentro de la
     * misma lectura de CellularSendCommand (conexión muy rápida).
     * Si ya está en 'response', no hace falta esperarla de nuevo.
     */
    if (strstr(response, expected) != NULL)
    {
        printf("CELLULAR: TCP connection established (URC llegó junto al OK)\n");
        return true;
    }

    printf("CELLULAR: QIOPEN accepted, waiting for connection result...\n");

    char urc_response[256];

    if (!CellularWaitForResponse(
            expected,
            urc_response,
            sizeof(urc_response),
            CELLULAR_QIOPEN_RESULT_TIMEOUT_MS))
    {
        printf("CELLULAR: TCP connection failed or timeout\n");
        return false;
    }

    printf("CELLULAR: TCP connection established\n");
    return true;
}







bool CellularWaitForResponse(
    const char *expected,
    char *response,
    size_t response_size,
    uint32_t timeout_ms
)
{
    if (expected == NULL ||
        response == NULL ||
        response_size == 0)
    {
        return false;
    }

    response[0] = '\0';

    uint32_t elapsed_ms = 0;

    while (elapsed_ms < timeout_ms)
    {
        char buffer[128];

        int len = UartHalReadBytes(
            buffer,
            sizeof(buffer) - 1,
            CELLULAR_AT_TIMEOUT_MS
        );

        if (len > 0)
        {
            buffer[len] = '\0';

            printf("CELLULAR WAIT RX: %s\n", buffer);

            //printf("CELLULAR URC RX: %s\n", buffer);

            /*
             * Agregamos los datos recibidos al buffer
             * acumulado.
             */
            size_t current_length = strlen(response);

            if (current_length + len < response_size)
            {
                memcpy(
                    response + current_length,
                    buffer,
                    len
                );

                response[current_length + len] = '\0';
            }

            /*
             * ¿Llegó la respuesta que estábamos esperando?
             */
            if (strstr(response, expected) != NULL)
            {
                printf(
                    "CELLULAR: Expected response received: %s\n",
                    expected
                );

                return true;
            }
        }

        elapsed_ms += CELLULAR_AT_TIMEOUT_MS;
    }

    printf(
        "CELLULAR: Timeout waiting for: %s\n",
        expected
    );

    return false;
}


/**
 * Consulta el estado de los sockets vía AT+QISTATE? y lo imprime por consola.
 * Devuelve true si pudo hacer la consulta (independientemente de si hay
 * sockets abiertos o no).
 */
bool CellularPrintSocketState(void)
{
    char response[256];

    printf("\n========== QISTATE (sockets activos) ==========\n");

    bool ok = CellularSendCommand(
        "AT+QISTATE?\r\n",
        response,
        sizeof(response),
        5000
    );

    if (!ok)
    {
        printf("No se pudo consultar QISTATE.\n");
        return false;
    }

    if (strstr(response, "+QISTATE:") == NULL)
    {
        printf("No hay sockets abiertos.\n");
    }
    else
    {
        printf("%s\n", response);
    }

    return true;
}


/**
 * Cierra (best-effort) todos los connectID posibles (0..11 en el EG915U-LA).
 * Se ignoran errores individuales: es normal que la mayoría no estén
 * abiertos y el módulo responda ERROR para esos casos.
 */
void CellularCloseAllSockets(void)
{
    char command[32];
    char response[128];

    printf("\n========== Cerrando sockets (barrido QICLOSE) ==========\n");

    for (int id = 0; id <= 11; id++)
    {
        snprintf(command, sizeof(command), "AT+QICLOSE=%d\r\n", id);
        CellularSendCommand(command, response, sizeof(response), 2000);
    }
}

/////////////////////////////////////
bool CellularInit(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" CELLULAR INIT\n");
    printf("========================================\n");

    if (!CellularPowerOn()) {
        printf("CELLULAR: Power ON fallo\n");
        return false;
    }

    if (!CellularWaitReady(12000)) {
        printf("CELLULAR: no se recibio RDY\n");
        return false;
    }

    if (!CellularIsReady()) {
        printf("CELLULAR: AT no respondio OK\n");
        return false;
    }

    printf("CELLULAR: EG915U listo\n");

    return true;
}




bool CellularReset(void) {
    UartHalWriteBytes("AT+CFUN=1,1\r\n", 13);
    return true;
}


// Apagado lógico recomendado
bool CellularPowerOff(void) {
    UartHalWriteBytes("AT+QPOWD=1\r\n", strlen("AT+QPOWD=1\r\n"));
    char response[64];
    int len = UartHalReadBytes(response, sizeof(response) - 1, 5000);
    if (len > 0) {
        response[len] = '\0';
        return strstr(response, "POWER DOWN") != NULL;
    }
    return false;
}

// Apagado físico (hard) - solo emergencia
bool CellularPowerOffHard(void) {
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);
    GPIOOff(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(2000)); // mantener bajo ~2s
    GPIOOn(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(5000)); // esperar apagado
    return true;
}



bool CellularConnect(void) {
    char response[64];
    UartHalWriteBytes("AT+CREG?\r\n", 10);
    int len = UartHalReadBytes(response, sizeof(response) - 1, 2000);
    return (len > 0 && strstr(response, "+CREG: 0,1") != NULL);
}

bool CellularDisconnect(void) {
    return true;
}


/*
bool CellularPdpConfigure(const char *apn) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",,,1\r\n", apn);
    // 1. Transmitir por TX (GPIO 18) 
    UartHalWriteBytes(cmd, strlen(cmd));

    char response[64];
    // 2. Intentar leer por RX (GPIO 19) 
    int len = UartHalReadBytes(response, sizeof(response) - 1, 2000);

    /////////////////////////////////////////////////

    
    if (len > 0) {
        response[len] = '\0'; // Asegurar fin de cadena 
    //    ESP_LOGI(TAG, ">>> ECO DETECTADO EN RX (%d bytes): %s", len, response);
    } else {
    //    ESP_LOGE(TAG, ">>> NO SE RECIBIÓ NADA EN RX (Timeout/Circuito abierto)");
    }
    
    /////////////////////////////////////////////////



    return (len > 0 && strstr(response, "OK") != NULL);
}

*/
/*
bool CellularPdpActivate(void) {
    UartHalWriteBytes("AT+QIACT=1\r\n", 12);
    char response[64];
    int len = UartHalReadBytes(response, sizeof(response) - 1, 5000);
    return (len > 0 && strstr(response, "OK") != NULL);
}

bool CellularGetImei(char *imei_out, size_t max_len) {
    if (!imei_out || max_len < 16) return false;
    UartHalWriteBytes("AT+GSN\r\n", 8);
    char response[64];
    int len = UartHalReadBytes(response, sizeof(response) - 1, 2000);
    if (len > 0) {
        response[len] = '\0';
        // En una implementación completa se extrae la línea numérica del IMEI.
        snprintf(imei_out, max_len, "861234567890123");
        return true;
    }
    return false;
}

bool CellularGetNtpTime(char *timestamp_out, size_t max_len) {
    if (!timestamp_out) return false;
    // Consulta NTP mediante comandos AT del Quectel y formato de fecha/hora[cite: 1]
    snprintf(timestamp_out, max_len, "2026-06-18 12:00:00");
    return true;
}

bool CellularTcpConnect(const char *host, uint16_t port) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,0,\"TCP\",\"%s\",%d,0,2\r\n", host, port);
    UartHalWriteBytes(cmd, strlen(cmd));
    char response[128];
    int len = UartHalReadBytes(response, sizeof(response) - 1, 5000);
    return (len > 0 && strstr(response, "+QIOPEN: 0,0") != NULL);
}

bool CellularTcpSend(const uint8_t *buffer, size_t length) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QISEND=0,%u\r\n", (unsigned int)length);
    UartHalWriteBytes(cmd, strlen(cmd));
    
    char prompt[8];
    int len = UartHalReadBytes(prompt, sizeof(prompt) - 1, 1000);
    if (len > 0 && strchr(prompt, '>') != NULL) {
        UartHalWriteBytes((const char *)buffer, length);
        char response[64];
        int r_len = UartHalReadBytes(response, sizeof(response) - 1, 3000);
        return (r_len > 0 && strstr(response, "SEND OK") != NULL);
    }
    return false;
}

int CellularTcpReceive(uint8_t *buffer, size_t length, uint32_t timeout_ms) {
    return UartHalReadBytes((char *)buffer, length, timeout_ms);
}

bool CellularTcpDisconnect(void) {
    UartHalWriteBytes("AT+QICLOSE=0\r\n", 14);
    return true;
}

bool CellularSmsSend(const char *number, const char *message) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"\r\n", number);
    UartHalWriteBytes(cmd, strlen(cmd));
    
    UartHalWriteBytes(message, strlen(message));
    char ctrl_z = 0x1A;
    UartHalWriteBytes(&ctrl_z, 1);
    
    char response[64];
    int len = UartHalReadBytes(response, sizeof(response) - 1, 10000);
    return (len > 0 && strstr(response, "+CMGS:") != NULL);
}
*/