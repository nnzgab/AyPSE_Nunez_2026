#include <stdio.h>
#include "unity.h"
#include "panic_handler.h"
#include "panic_button.h"
#include "board_clock.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static volatile uint16_t s_last_sequence_received = 0;
static volatile int      s_callback_call_count = 0;

/* Callback de prueba para capturar los eventos del handler */
static void TestPanicCallback(uint16_t sequence_number) {
    s_last_sequence_received = sequence_number;
    s_callback_call_count++;
    printf("\n[CALLBACK TEST] ¡Evento de pánico recibido! Secuencia #: %u\n", sequence_number);
}

/* ============================================================================
 * TEST-MW-PANIC-01: Validación de parámetros e inicialización limpia
 * ============================================================================ */
TEST_CASE("TEST-MW-PANIC-01 PanicHandler_Init validation without RTOS", "[middleware][panic_handler]") {
    printf("\n========================================\n");
    printf(" TEST-MW-PANIC-01 INICIALIZACION C PURO\n");
    printf("========================================\n");

    // 1. Rechazo de callback nulo
    TEST_ASSERT_EQUAL_INT_MESSAGE(PANIC_HANDLER_ERR_PARAM, PanicHandler_Init(NULL),
        "PanicHandler_Init(NULL) debió devolver PANIC_HANDLER_ERR_PARAM");

    // 2. Inicialización válida
    TEST_ASSERT_EQUAL_INT_MESSAGE(PANIC_HANDLER_OK, PanicHandler_Init(TestPanicCallback),
        "PanicHandler_Init debió devolver PANIC_HANDLER_OK");

    printf("--> Inicialización C puro (sin FreeRTOS) completada correctamente.\n");
}

/* ============================================================================
 * TEST-MW-PANIC-02: Respuesta a botón físico mediante RunStep no bloqueante
 * ============================================================================ */
TEST_CASE("TEST-MW-PANIC-02 PanicHandler_RunStep processes button press", "[middleware][panic_handler][interactive]") {
    printf("\n========================================\n");
    printf(" TEST-MW-PANIC-02 DETECCION INTERACTIVA CON RUNSTEP\n");
    printf("========================================\n");

    s_callback_call_count = 0;
    s_last_sequence_received = 0;

    TEST_ASSERT_EQUAL_INT(PANIC_HANDLER_OK, PanicHandler_Init(TestPanicCallback));

    printf("\n========================================\n");
    printf(" MANTENGA PRESIONADO EL BOTON DE PANICO\n");
    printf(" durante al menos 1 segundo ahora...\n");
    printf("========================================\n");

    uint32_t start_ms = BoardClockGetMs();
    while ((BoardClockGetMs() - start_ms) < 3000) {
        PanicHandler_RunStep();
        // Delay corto para no saturar el bucle de test
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    TEST_ASSERT_GREATER_THAN_INT_MESSAGE(0, s_callback_call_count,
        "No se capturó ninguna pulsación durante el ciclo de prueba");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(1, s_last_sequence_received,
        "La primera secuencia debió ser #1");

    printf("--> Evento detectado y procesado sin dependencias de FreeRTOS en el módulo.\n");
}

/* ============================================================================
 * TEST-MW-PANIC-03: Pulsaciones múltiples y secuencia incremental
 * ============================================================================ */
TEST_CASE("TEST-MW-PANIC-03 PanicHandler multiple presses with RunStep", "[middleware][panic_handler][interactive]") {
    printf("\n========================================\n");
    printf(" TEST-MW-PANIC-03 PULSACIONES MULTIPLES Y SECUENCIA\n");
    printf("========================================\n");

    s_callback_call_count = 0;
    s_last_sequence_received = 0;

    TEST_ASSERT_EQUAL_INT(PANIC_HANDLER_OK, PanicHandler_Init(TestPanicCallback));

    printf("\n========================================\n");
    printf(" PRESIONE EL BOTON 3 VECES\n");
    printf(" con breves pausas entre cada pulsación (8s disponibles)...\n");
    printf("========================================\n");

    uint32_t start_ms = BoardClockGetMs();
    while ((BoardClockGetMs() - start_ms) < 8000) {
        PanicHandler_RunStep();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    printf("\nTotal de llamadas al callback: %d (esperado: 3)\n", s_callback_call_count);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, s_callback_call_count,
        "Se esperaban exactamente 3 llamadas al callback");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(3, s_last_sequence_received,
        "La última secuencia debió ser #3");

    printf("--> Secuencia incremental y debounce verificados exitosamente.\n");
}