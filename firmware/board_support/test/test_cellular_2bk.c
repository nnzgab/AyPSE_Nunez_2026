#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "cellular_modem.h"
#include "uart_hal.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TEST_APN       "datos.personal.com"
#define TEST_USER      "datos"
#define TEST_PASSWORD      "datos"

TEST_CASE("TEST-BSP-Secuencia-Completa-EG915U", "[bsptest][cellular]")
{
    char ip_buf[32] = {0};

    printf("\n========================================\n");
    printf(" INICIO DE PRUEBA BSP - QUECTEL EG915U\n");
    printf("========================================\n");

    /* Paso 0: Inicialización de hardware (UART + GPIOs) */
    printf("[PASO 0] Inicializando BSP...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemInit(), "FAIL: Error al inicializar UART/GPIOs");

    /* Paso 1: Pulso físico de encendido */
    printf("[PASO 1] Aplicando pulso PWRKEY...\n");
    CellularModemPowerPulse();

    /* Paso 2: Espera pasiva de URC 'RDY' */
    printf("[PASO 2] Esperando URC 'RDY' de arranque...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemWaitBoot(6000), "FAIL: Timeout esperando RDY");

    /* Paso 3: Verificación de canal AT */
    printf("[PASO 3] Consultando respuesta AT...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsReady(), "FAIL: El módem no responde AT");

    /* Paso 4: Estado de la tarjeta SIM */
    printf("[PASO 4] Verificando estado de la SIM (AT+CPIN?)...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsSimReady(), "FAIL: La SIM no está lista (+CPIN)");

    /* Paso 5: Registro en la red celular */
    printf("[PASO 5] Consultando registro en red (AT+CEREG?)...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsNetworkRegistered(), "FAIL: No registrado en red celular");

    /* Paso 6: Configuración del contexto PDP */
    printf("[PASO 6] Configurando APN '%s' (AT+QICSGP)...\n", TEST_APN);
    TEST_ASSERT_TRUE_MESSAGE(CellularModemConfigurePdp(TEST_APN, TEST_USER, TEST_PASSWORD), 
                             "FAIL: Error al configurar PDP");

    /* Paso 7: Activación del contexto PDP */
    printf("[PASO 7] Activando contexto PDP (AT+QIACT=1)... esperá hasta 15s...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemActivatePdp(), "FAIL: Falló la activación PDP");

    /* Paso 8: Confirmación de PDP activo y extracción de IP */
    printf("[PASO 8] Verificando IP asignada (AT+QIACT?)...\n");
    TEST_ASSERT_TRUE_MESSAGE(CellularModemIsPdpActive(ip_buf, sizeof(ip_buf)), 
                             "FAIL: El PDP no figura activo o error leyendo IP");

    printf("\n========================================\n");
    printf(" CONEXIÓN EXITOSA! IP Asignada: %s\n", ip_buf);
    printf("========================================\n");
}



TEST_CASE("TEST-BSP-MODEM-04 CellularModemHardPowerOff verification", "[apagar][cellular][interactive]")
{
    printf("\n[ATENCION] Iniciando secuencia de apagado duro...\n");
    
    // 1. Ejecutar el apagado por hardware
    CellularModemHardPowerOff();
    
    // 2. Cuenta regresiva visible de estabilización (5 segundos)
    for (int i = 5; i > 0; i--) {
        printf("Esperando estabilizacion post apagado... %d segundos restantes\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("\nVerificando que el modem haya quedado mudo en la UART...\n");
    const char *cmd = "AT\r\n";
    
    // Transmitir comando por UART
    UartHalWriteBytes(cmd, strlen(cmd));
    
    uint8_t buffer[64] = {0};
    int read_bytes = UartHalReadBytes((char *)buffer, sizeof(buffer) - 1, 1000);
    
    // Si no responde o no devuelve OK, el apagado fue exitoso
    if (read_bytes <= 0 || strstr((char *)buffer, "OK") == NULL) {
        printf("--> ¡Exito! El modem esta completamente apagado (Hard Off verificado).\n");
        TEST_PASS();
    } else {
        printf("--> [FALLO] El modem sigue respondiendo: %s\n", buffer);
        TEST_FAIL_MESSAGE("El apagado duro no surtio efecto en el hardware.");
    }
}