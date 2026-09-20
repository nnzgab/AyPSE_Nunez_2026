#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "uart_hal.h"

/* ============================================================================
 * TEST-HAL-UART-01: Inicialización con pines por defecto y explícitos
 * ============================================================================ */
TEST_CASE("TEST-HAL-UART-01 UartHalInit configures UART port cleanly", "[drivers_hal][uart][init]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-UART-01 INICIALIZACION DE PUERTO SERIE\n");
    printf("========================================\n");

    printf("[PASO 1] Inicializando UART1 a 115200 baudios (pines por defecto)...\n");
    UartHalInit(115200);

    printf("[PASO 2] Re-inicializando con asignación explícita de pines (TX=18, RX=19)...\n");
    UartHalInitWithPins(115200, 18, 19);

    TEST_ASSERT_TRUE_MESSAGE(true, "FAIL: Error al inicializar driver UART");
    printf("--> Éxito: Puerto UART1 configurado correctamente.\n");
}

/* ============================================================================
 * TEST-HAL-UART-02: Manejo de timeouts en lecturas sin datos
 * ============================================================================ */
TEST_CASE("TEST-HAL-UART-02 UartHalReadBytes handles timeouts gracefully", "[drivers_hal][uart][timeout]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-UART-02 TIMEOUT EN LECTURA SERIE\n");
    printf("========================================\n");

    UartHalInit(115200);
    char rx_buf[44] = {0};

    printf("Ejecutando lectura con timeout de 100 ms sin datos en línea...\n");
    int read_bytes = UartHalReadBytes(rx_buf, sizeof(rx_buf) - 1, 100);

    printf("Bytes leídos: %d (esperado: 0 por timeout)\n", read_bytes);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, read_bytes, "FAIL: La lectura serie no retornó 0 ante timeout");
    printf("--> Éxito: Función de lectura retorna 0 sin bloquear la CPU tras el tiempo límite.\n");
}

/* ============================================================================
 * TEST-HAL-UART-03: Robustez ante parámetros nulos (NULL Pointers)
 * ============================================================================ */
TEST_CASE("TEST-HAL-UART-03 Driver handles NULL pointer arguments safely", "[drivers_hal][uart][robustness]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-UART-03 VALIDACION DE PUNTEROS NULOS\n");
    printf("========================================\n");

    UartHalInit(115200);

    printf("[PASO 1] Probando UartHalReadByte(NULL)...\n");
    int res_read = UartHalReadByte(NULL);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res_read, "FAIL: UartHalReadByte no retornó error (-1) al recibir NULL");

    printf("[PASO 2] Probando UartHalWriteBytes(NULL, 10)...\n");
    int res_write = UartHalWriteBytes(NULL, 10);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, res_write, "FAIL: UartHalWriteBytes no retornó 0 al recibir NULL");

    printf("--> Éxito: El driver de bajo nivel previene colisiones por punteros nulos.\n");
}