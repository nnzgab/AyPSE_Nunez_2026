
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

#define CELLULAR_PWRKEY_ON_TIME_MS       100
#define CELLULAR_PWRKEY_OFF_TIME_MS      2000
#define CELLULAR_BOOT_WAIT_MS            5000

#define CELLULAR_AT_TIMEOUT_MS           1000
#define CELLULAR_POWEROFF_TIMEOUT_MS     5000
#define CELLULAR_CFUN_TIMEOUT_MS    10000

#define CELLULAR_NETWORK_POLL_MS       1000
#define CELLULAR_NETWORK_TIMEOUT_MS    120000




bool CellularPowerOn(void)//*********************** */
{
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);
    GPIOOff(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(CELLULAR_PWRKEY_ON_TIME_MS));
    GPIOOn(QUECTEL_PWRKEY_PIN);
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

 bool CellularSendCommand( const char *command, char *response, size_t response_size, uint32_t timeout_ms);

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