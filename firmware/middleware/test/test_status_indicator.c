#include <stdio.h>
#include "unity.h"
#include "status_indicator.h"
#include "board_config.h"
#include "gpio_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board_clock.h"

#define DELAY_MARGIN_MS  50

/* ============================================================================
 * TEST-MW-STAT-01: Inicialización del Indicador de Estado
 * ============================================================================ */
TEST_CASE("TEST-MW-STAT-01 StatusIndicator_Init initializes LEDs and default state", "[middleware][status_indicator]") {
    printf("\n========================================\n");
    printf(" TEST-MW-STAT-01 INICIALIZACION DE STATUS INDICATOR\n");
    printf("========================================\n");

    bool ok = StatusIndicator_Init();
    TEST_ASSERT_TRUE_MESSAGE(ok, "StatusIndicator_Init() devolvió false");

    // Verificar estado inicial en OFF
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELLULAR_STATUS_OFF, StatusIndicator_GetCellular(),
        "El estado inicial debe ser CELLULAR_STATUS_OFF");

    // Verificar que los LEDs comiencen físicamente apagados
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_PANIC_LED_STATUS), "LED Panic no está APAGADO");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_QUECTEL_LED_STATUS), "LED Quectel no está APAGADO");

    printf("--> Inicialización correcta: LEDs en OFF y estado CELLULAR_STATUS_OFF.\n");
}

/* ============================================================================
 * TEST-MW-STAT-02: Control del LED de Pánico (SetPanic)
 * ============================================================================ */
TEST_CASE("TEST-MW-STAT-02 StatusIndicator_SetPanic controls PANIC LED directly", "[middleware][status_indicator]") {
    printf("\n========================================\n");
    printf(" TEST-MW-STAT-02 CONTROL DE LED DE PANICO\n");
    printf("========================================\n");

    StatusIndicator_Init();

    printf("Activando indicador de pánico (SetPanic = true)...\n");
    StatusIndicator_SetPanic(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_PANIC_LED_STATUS),
        "Error: LED de Pánico debería estar ENCENDIDO (1)");

    printf("Desactivando indicador de pánico (SetPanic = false)...\n");
    StatusIndicator_SetPanic(false);
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_PANIC_LED_STATUS),
        "Error: LED de Pánico debería estar APAGADO (0)");

    printf("--> Control de LED de pánico verificado exitosamente.\n");
}

/* ============================================================================
 * TEST-MW-STAT-03: Asignación y Consulta de Estados de Red (SetCellular / GetCellular)
 * ============================================================================ */
TEST_CASE("TEST-MW-STAT-03 StatusIndicator_SetCellular sets state and initial LED status", "[middleware][status_indicator]") {
    printf("\n========================================\n");
    printf(" TEST-MW-STAT-03 ASIGNACION DE ESTADOS DE RED\n");
    printf("========================================\n");

    StatusIndicator_Init();

    printf("Estableciendo estado CELLULAR_STATUS_STARTING...\n");
    StatusIndicator_SetCellular(CELLULAR_STATUS_STARTING);
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELLULAR_STATUS_STARTING, StatusIndicator_GetCellular(),
        "GetCellular debe devolver CELLULAR_STATUS_STARTING");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_QUECTEL_LED_STATUS),
        "LED Quectel debería estar encendido fijo en STARTING");

    printf("Estableciendo estado CELLULAR_STATUS_OFF...\n");
    StatusIndicator_SetCellular(CELLULAR_STATUS_OFF);
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELLULAR_STATUS_OFF, StatusIndicator_GetCellular(),
        "GetCellular debe devolver CELLULAR_STATUS_OFF");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_QUECTEL_LED_STATUS),
        "LED Quectel debería estar apagado en OFF");

    printf("--> Estados estáticos (STARTING y OFF) verificados correctamente.\n");
}

/* ============================================================================
 * TEST-MW-STAT-04: Temporización y observación visual de patrones
 * ============================================================================ */
TEST_CASE("TEST-MW-STAT-04 Visual inspection of LED blinking patterns", "[status_indicator][visual]") {
    printf("\n========================================\n");
    printf(" TEST-MW-STAT-04 INSPECCION VISUAL DE PATRONES\n");
    printf("========================================\n");

    StatusIndicator_Init();

    /* 1. Patrón SEARCHING (200 ms ON / 1800 ms OFF) - Ejecuta por 6 segundos */
    printf("\n[1/3] Observá LED QUECTEL: Destello CORTO cada 2 segundos (BUSCANDO RED)...\n");
    StatusIndicator_SetCellular(CELLULAR_STATUS_SEARCHING);
    
    uint32_t start_ms = BoardClockGetMs();
    while ((BoardClockGetMs() - start_ms) < 6000) {
        StatusIndicator_RunStep();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* 2. Patrón READY (1800 ms ON / 200 ms OFF) - Ejecuta por 6 segundos */
    printf("\n[2/3] Observá LED QUECTEL: Casi siempre ENCENDIDO con guioñ corto OFF (LISTO/RED)...\n");
    StatusIndicator_SetCellular(CELLULAR_STATUS_READY);
    
    start_ms = BoardClockGetMs();
    while ((BoardClockGetMs() - start_ms) < 6000) {
        StatusIndicator_RunStep();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* 3. Patrón TRANSMITTING (125 ms ON / 125 ms OFF) - Ejecuta por 4 segundos */
    printf("\n[3/3] Observá LED QUECTEL: Parpadeo RAPIDO 4Hz (TRANSMITIENDO DATOS)...\n");
    StatusIndicator_SetCellular(CELLULAR_STATUS_TRANSMITTING);
    
    start_ms = BoardClockGetMs();
    while ((BoardClockGetMs() - start_ms) < 4000) {
        StatusIndicator_RunStep();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Dejar apagado al finalizar */
    StatusIndicator_SetCellular(CELLULAR_STATUS_OFF);
    printf("\n---> Prueba visual finalizada exitosamente.\n");
}