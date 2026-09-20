#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "cellular_modem.h"
#include "uart_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ============================================================================
 * Configuración de Red, APN y Servidor de Prueba
 * ============================================================================ */
#define TEST_APN                    "datos.personal.com"
#define TEST_USER                   "datos"
#define TEST_PASSWORD               "datos"

/* Reemplazar con los datos provistos por Pinggy o tu servidor de prueba */
#define TEST_SERVER_IP              "kkgqa-190-183-23-94.run.pinggy-free.link"
#define TEST_SERVER_PORT            41243
#define TEST_SOCKET_PROTO           "TCP"

/* Tiempo de espera amplio (30s) para permitir respuesta manual en el servidor */
#define MANUAL_RESPONSE_TIMEOUT_MS  30000

/* ============================================================================
 * TEST-BSP-MODEM-01: Secuencia Completa de Inicialización y Red
 * ============================================================================ */
TEST_CASE("TEST-BSP-MODEM-01 Secuencia completa de arranque y registro PDP", "[bsp][cellular][init]") {
    char ip_buf[32] = {0};

    printf("\n========================================\n");
    printf(" TEST-BSP-MODEM-01 INICIALIZACION Y RED\n");
    printf("========================================\n");

    /* Paso 0: Inicialización de hardware (UART + GPIOs) */
    printf("[PASO 0] Inicializando BSP del modem...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemInit(), "FAIL: Error al inicializar UART/GPIOs");

    /* Paso 1: Pulso físico de encendido */
    printf("[PASO 1] Aplicando pulso PWRKEY...\n");
    CellularModemPowerPulse();

    /* Paso 2: Espera de URC 'RDY' de arranque */
    printf("[PASO 2] Esperando URC 'RDY' de arranque...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemWaitBoot(6000), "FAIL: Timeout esperando RDY");

    /* Paso 3: Verificación de canal AT */
    printf("[PASO 3] Consultando respuesta AT...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsReady(), "FAIL: El modem no responde AT");

    /* Paso 4: Estado de la tarjeta SIM */
    printf("[PASO 4] Verificando estado de la SIM (AT+CPIN?)...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsSimReady(), "FAIL: La SIM no esta lista (+CPIN)");

    /* Paso 5: Registro en la red celular */
    printf("[PASO 5] Consultando registro en red (AT+CEREG?)...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsNetworkRegistered(), "FAIL: No registrado en red celular");

    /* Paso 6: Configuración del contexto PDP */
    printf("[PASO 6] Configurando APN '%s' (AT+QICSGP)...\n", TEST_APN);
    TEST_ASSERT_TRUE_MESSAGE(CellularModemConfigurePdp(TEST_APN, TEST_USER, TEST_PASSWORD), 
                             "FAIL: Error al configurar PDP");

    /* Paso 7: Activación del contexto PDP */
    printf("[PASO 7] Activando contexto PDP (AT+QIACT=1)...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemActivatePdp(), "FAIL: FAllo la activacion PDP");

    /* Paso 8: Confirmación de PDP activo y extracción de IP */
    printf("[PASO 8] Verificando IP asignada (AT+QIACT?)...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsPdpActive(ip_buf, sizeof(ip_buf)), 
                             "FAIL: El PDP no figura activo o error leyendo IP");

    printf("\n========================================\n");
    printf(" CONEXION EXITOSA! IP Asignada: %s\n", ip_buf);
    printf("========================================\n");
}

/* ============================================================================
 * TEST-BSP-MODEM-02: Lectura de Identificador IMEI
 * ============================================================================ */
TEST_CASE("TEST-BSP-MODEM-02 CellularModemGetIMEI reads 15-digit string", "[bsp][cellular][imei]") {
    printf("\n========================================\n");
    printf(" TEST-BSP-MODEM-02 LECTURA DE IMEI\n");
    printf("========================================\n");

    char imei[16] = {0};

    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsReady(), "FAIL: El modem no responde comandos AT");

    printf("Solicitando IMEI al modem (AT+CGSN)...\n");
    bool ok = CellularModemGetIMEI(imei, sizeof(imei));

    printf("IMEI Obtenido: %s\n", imei);

    TEST_ASSERT_TRUE_MESSAGE(ok, "FAIL: CellularModemGetIMEI devolvio false");
    TEST_ASSERT_EQUAL_INT_MESSAGE(15, strlen(imei), "FAIL: El IMEI debe tener exactamente 15 digitos");
}

/* ============================================================================
 * TEST-BSP-MODEM-03: Transmisión de IMEI y Recepción Manual de ACK por TCP
 * ============================================================================ */
TEST_CASE("TEST-BSP-MODEM-03 TCP Socket Transmission and Manual ACK Verification", "[bsp][cellular][socket]") {
    printf("\n========================================\n");
    printf(" TEST-BSP-MODEM-03 SOCKET TCP Y RECEPCION ACK\n");
    printf("========================================\n");

    char imei[16] = {0};
    uint8_t rx_buffer[64] = {0};
    uint16_t rx_bytes = 0;

    /* 1. Obtener IMEI para usarlo como trama de envio */
    TEST_ASSERT_TRUE_MESSAGE(CellularModemGetIMEI(imei, sizeof(imei)), "FAIL: No se pudo obtener el IMEI");

    /* 2. Apertura de Socket TCP */
    printf("Abriendo socket TCP hacia %s:%d...\n", TEST_SERVER_IP, TEST_SERVER_PORT);
    TEST_ASSERT_TRUE_MESSAGE(CellularModemSocketOpen(TEST_SOCKET_PROTO, TEST_SERVER_IP, TEST_SERVER_PORT),
                             "FAIL: Error al abrir socket TCP");

    /* 3. Envío del IMEI por TCP */
    printf("Enviando trama con IMEI (%s) por TCP...\n", imei);
    TEST_ASSERT_TRUE_MESSAGE(CellularModemSocketSend((const uint8_t *)imei, (uint16_t)strlen(imei)),
                             "FAIL: Error al transmitir datos por el socket");

    /* 4. Notificación para prueba manual */
    printf("\n==================================================\n");
    printf(" TRAMA ENVIADA CORRECTAMENTE (%s)\n", imei);
    printf(" -> Ingrese al servidor y responda 'OK'.\n");
    printf(" -> Tiempo maximo de espera: 30 segundos...\n");
    printf("==================================================\n");

    /* 5. Esperar y leer respuesta del servidor remoto */
    bool rx_ok = CellularModemSocketReceive(rx_buffer, sizeof(rx_buffer) - 1, &rx_bytes);
    
    if (rx_ok && rx_bytes > 0) {
        rx_buffer[rx_bytes] = '\0';
        printf("Respuesta recibida del servidor (%u bytes): %s\n", rx_bytes, (char *)rx_buffer);
    } else {
        printf("FAIL: Timeout o no se recibieron datos del servidor.\n");
    }

    /* 6. Cerrar el socket */
    CellularModemSocketClose();

    /* 7. Validaciones */
    TEST_ASSERT_TRUE_MESSAGE(rx_ok, "FAIL: CellularModemSocketReceive fallo o dio timeout");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr((char *)rx_buffer, "OK"), "FAIL: Se esperaba la respuesta 'OK' del servidor");

    printf("\n========================================\n");
    printf(" TEST DE SOCKET TCP FINALIZADO CON EXITO\n");
    printf("========================================\n");
}

/* ============================================================================
 * TEST-BSP-MODEM-04: Apagado Forzado por Hardware
 * ============================================================================ */
TEST_CASE("TEST-BSP-MODEM-04 CellularModemHardPowerOff verification", "[bsp][cellular][poweroff]") {
    printf("\n========================================\n");
    printf(" TEST-BSP-MODEM-04 APAGADO HARDWARE (HARD POWER OFF)\n");
    printf("========================================\n");

    /* 1. Ejecutar el apagado por hardware */
    CellularModemHardPowerOff();

    /* 2. Cuenta regresiva visible de estabilización */
    for (int i = 5; i > 0; i--) {
        printf("Esperando estabilizacion post apagado... %d segundos restantes\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("\nVerificando que el modem haya quedado mudo en la UART...\n");
    const char *cmd = "AT\r\n";
    UartHalWriteBytes(cmd, strlen(cmd));

    uint8_t buffer[64] = {0};
    int read_bytes = UartHalReadBytes((char *)buffer, sizeof(buffer) - 1, 1000);

    if (read_bytes <= 0 || strstr((char *)buffer, "OK") == NULL) {
        printf("--> Exito! El modem esta completamente apagado.\n");
        TEST_PASS();
    } else {
        TEST_FAIL_MESSAGE("FAIL: El modem sigue respondiendo a comandos AT.");
    }
}