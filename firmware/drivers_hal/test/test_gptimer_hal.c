#include <stdio.h>

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gptimer_hal.h"

/*==================[TEST-01]==================================================*/
TEST_CASE("TEST-01 GpTimerInit returns OK and is idempotent", "[gptimer][init]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-01 GPTIMER INIT\n");
    printf("========================================\n");

    int8_t ret1 = GpTimerInit();
    TEST_ASSERT_EQUAL_INT8_MESSAGE(HAL_GPTIMER_OK, ret1,
        "La primera inicializacion deberia devolver HAL_GPTIMER_OK");

    /* Segunda llamada: debe ser idempotente, sin reiniciar el conteo */
    uint32_t before = GpTimerGetMs();
    int8_t ret2 = GpTimerInit();
    TEST_ASSERT_EQUAL_INT8_MESSAGE(HAL_GPTIMER_OK, ret2,
        "La segunda inicializacion deberia devolver HAL_GPTIMER_OK sin reconfigurar");

    uint32_t after = GpTimerGetMs();
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(before, after,
        "El conteo no deberia reiniciarse en una segunda inicializacion");
}

/*==================[TEST-02]==================================================*/
TEST_CASE("TEST-02 GpTimerGetMs advances consistently with real elapsed time", "[gptimer][timing]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-02 GPTIMER GETMS\n");
    printf("========================================\n");

    GpTimerInit();

    uint32_t t0 = GpTimerGetMs();
    vTaskDelay(pdMS_TO_TICKS(500));
    uint32_t t1 = GpTimerGetMs();
    uint32_t elapsed = t1 - t0;

    printf("Tiempo medido: %lu ms (esperado ~500 ms)\n", (unsigned long)elapsed);

    /* Tolerancia amplia por jitter del scheduler de FreeRTOS */
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(450, elapsed,
        "El tiempo medido es menor al esperado");
    TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(600, elapsed,
        "El tiempo medido es mayor al esperado");
}

/*==================[TEST-03]==================================================*/
TEST_CASE("TEST-03 GpTimerDelayMs blocks approximately the requested time", "[gptimer][timing]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-03 GPTIMER DELAYMS\n");
    printf("========================================\n");

    GpTimerInit();

    uint32_t t0 = GpTimerGetMs();
    GpTimerDelayMs(200);
    uint32_t t1 = GpTimerGetMs();
    uint32_t elapsed = t1 - t0;

    printf("Tiempo bloqueado: %lu ms (esperado ~200 ms)\n", (unsigned long)elapsed);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(195, elapsed,
        "El busy-wait termino antes de tiempo");
    TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(250, elapsed,
        "El busy-wait se extendio mas de lo esperado");
}

/*==================[TEST-04]==================================================*/
TEST_CASE("TEST-04 GpTimerDelayMs with zero returns immediately", "[gptimer][timing]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-04 GPTIMER DELAYMS(0)\n");
    printf("========================================\n");

    GpTimerInit();

    uint32_t t0 = GpTimerGetMs();
    GpTimerDelayMs(0);
    uint32_t t1 = GpTimerGetMs();

    printf("Delta con ms=0: %lu ms\n", (unsigned long)(t1 - t0));

    TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(5, (t1 - t0),
        "GpTimerDelayMs(0) no deberia bloquear de forma perceptible");
}

/*==================[end of file]============================================*/