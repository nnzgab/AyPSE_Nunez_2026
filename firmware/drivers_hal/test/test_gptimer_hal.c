#include <stdio.h>
#include "unity.h"
#include "gptimer_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ============================================================================
 * TEST-HAL-TIMER-01: Inicialización del Hardware Timer
 * ============================================================================ */
TEST_CASE("TEST-HAL-TIMER-01 GpTimerInit initializes hardware timer", "[drivers_hal][gptimer][init]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-TIMER-01 INICIALIZACION GPTIMER\n");
    printf("========================================\n");

    int8_t status = GpTimerInit();
    TEST_ASSERT_EQUAL_INT_MESSAGE(HAL_GPTIMER_OK, status, "FAIL: GpTimerInit devolvió código de error");
    printf("--> Éxito: Timer de hardware a 1 MHz (1 us/tick) iniciado con éxito.\n");
}

/* ============================================================================
 * TEST-HAL-TIMER-02: Avance consistente de la marca de tiempo (GpTimerGetMs)
 * ============================================================================ */
TEST_CASE("TEST-HAL-TIMER-02 GpTimerGetMs advances consistently", "[drivers_hal][gptimer][timing]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-TIMER-02 CONTEO DE TIEMPO (GpTimerGetMs)\n");
    printf("========================================\n");

    GpTimerInit();
    uint32_t t0 = GpTimerGetMs();
    vTaskDelay(pdMS_TO_TICKS(500));
    uint32_t t1 = GpTimerGetMs();
    uint32_t elapsed = t1 - t0;

    printf("Tiempo transcurrido registrado: %lu ms (esperado ~500 ms)\n", (unsigned long)elapsed);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(450, elapsed, "FAIL: Tiempo medido menor al rango permitido");
    TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(600, elapsed, "FAIL: Tiempo medido superó la tolerancia");
    printf("--> Éxito: Medición de tiempo coincidente con la ventana de retardo.\n");
}

/* ============================================================================
 * TEST-HAL-TIMER-03: Retardo activo sin ceder procesador (GpTimerDelayMs)
 * ============================================================================ */
TEST_CASE("TEST-HAL-TIMER-03 GpTimerDelayMs blocks for requested time", "[drivers_hal][gptimer][delay]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-TIMER-03 RETARDO ACTIVO (GpTimerDelayMs)\n");
    printf("========================================\n");

    GpTimerInit();
    uint32_t t0 = GpTimerGetMs();
    GpTimerDelayMs(200);
    uint32_t t1 = GpTimerGetMs();
    uint32_t elapsed = t1 - t0;

    printf("Tiempo bloqueado en busy-wait: %lu ms (esperado ~200 ms)\n", (unsigned long)elapsed);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(195, elapsed, "FAIL: El retardo finalizó antes de tiempo");
    TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(250, elapsed, "FAIL: El retardo se extendió más de lo permitido");
    printf("--> Éxito: Demora activa ajustada a la precisión de milisegundos.\n");
}