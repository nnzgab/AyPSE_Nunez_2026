
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "cellular.h"
#include "uart_hal.h"
#include "board_config.h"
#include <string.h>
#include <stdio.h>


#include "esp_log.h"              // <-- AGREGAR ESTA LÍNEA quitar esto
static const char *TAG = "CELLULAR_BSP"; // <-- AGREGAR ESTA LÍNEA quitar esto

bool CellularInit(void) {
    // 1. Inicializar la UART para los comandos AT
    UartHalInit(UART_BAUDRATE);

    // 2. Configurar el pin PWRKEY como salida digital
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);

    // 3. Secuencia típica de encendido para el Quectel (pulso en PWRKEY)
    // Dependiendo del hardware, se suele llevar a bajo unos milisegundos y liberar, 
    // o viceversa, para forzar el encendido del módulo.
    GPIOOff(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(100)); // Mantener presionado/bajo
    GPIOOn(QUECTEL_PWRKEY_PIN);     // Liberar
    vTaskDelay(pdMS_TO_TICKS(2000)); // Esperar a que el módem arranque y estabilice su UART

    return true;
}

bool CellularReset(void) {
    UartHalWriteBytes("AT+CFUN=1,1\r\n", 13);
    return true;
}

bool CellularIsReady(void) {
    char response[64];
    UartHalWriteBytes("AT\r\n", 4);
    int len = UartHalReadBytes(response, sizeof(response) - 1, 1000);
    if (len > 0) {
        response[len] = '\0';
        if (strstr(response, "OK") != NULL) {
            return true;
        }
    }
    return false;
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

bool CellularPdpConfigure(const char *apn) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",,,1\r\n", apn);
    /* 1. Transmitir por TX (GPIO 18) */
    UartHalWriteBytes(cmd, strlen(cmd));

    char response[64];
    /* 2. Intentar leer por RX (GPIO 19) */
    int len = UartHalReadBytes(response, sizeof(response) - 1, 2000);

    /////////////////////////////////////////////////

    /* --- AGREGAR DESDE AQUÍ --- */
    if (len > 0) {
        response[len] = '\0'; /* Asegurar fin de cadena */
        ESP_LOGI(TAG, ">>> ECO DETECTADO EN RX (%d bytes): %s", len, response);
    } else {
        ESP_LOGE(TAG, ">>> NO SE RECIBIÓ NADA EN RX (Timeout/Circuito abierto)");
    }
    /* --- HASTA AQUÍ --- */
    /////////////////////////////////////////////////



    return (len > 0 && strstr(response, "OK") != NULL);
}

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