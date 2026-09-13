#include <stdio.h>

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "status_indicator.h"
#include "board_clock.h"

/*==================[internal functions definition]==============================*/

/**
 * @brief Corre StatusIndicator_RunStep() en un loop de polling durante
 * duration_ms, con un período de sondeo bien por debajo del semiciclo más
 * corto (TRANSMITTING = 125 ms), para que el parpateo se vea fluido.
 */
static void run_status_indicator_for(uint32_t duration_ms)
{
    uint32_t start = BoardClockGetMs();

    while ((BoardClockGetMs() - start) < duration_ms) {
        StatusIndicator_RunStep();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/*==================[TEST-MW-01]===============================================*/
TEST_CASE("TEST-MW-01 StatusIndicator_Init initializes without error", "[status_indicator][init]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-01 STATUS INDICATOR INIT\n");
    printf("========================================\n");

    bool ok = StatusIndicator_Init();
    TEST_ASSERT_TRUE_MESSAGE(ok, "StatusIndicator_Init deberia devolver true");
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELLULAR_STATUS_OFF, StatusIndicator_GetCellular(),
        "El estado inicial deberia ser CELLULAR_STATUS_OFF");
}

/*==================[TEST-MW-02]===============================================*/
TEST_CASE("TEST-MW-02 StatusIndicator_SetPanic controls LED_PANIC", "[status_indicator][panic]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-02 PANIC LED\n");
    printf("========================================\n");

    StatusIndicator_Init();

    printf("\n>>> OBSERVE LED_PANIC: deberia ENCENDERSE fijo ahora <<<\n");
    StatusIndicator_SetPanic(true);
    vTaskDelay(pdMS_TO_TICKS(3000));

    printf(">>> OBSERVE LED_PANIC: deberia APAGARSE ahora <<<\n");
    StatusIndicator_SetPanic(false);
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* No hay readback de hardware: este test es de confirmacion visual.
     * Si llego hasta aca sin crashear, se considera aprobado a nivel API. */
    TEST_PASS_MESSAGE("Confirmar visualmente que LED_PANIC encendio y apago segun lo indicado");
}

/*==================[TEST-MW-03]===============================================*/
TEST_CASE("TEST-MW-03 SEARCHING pattern: LED_QUECTEL 200ms ON / 1800ms OFF", "[status_indicator][blink]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-03 SEARCHING PATTERN\n");
    printf("========================================\n");
    printf(">>> OBSERVE LED_QUECTEL: destello corto, pausa larga <<<\n");
    printf(">>> (aprox. un parpadeo breve cada 2 segundos) <<<\n");

    StatusIndicator_Init();
    StatusIndicator_SetCellular(CELLULAR_STATUS_SEARCHING);

    run_status_indicator_for(8000);

    TEST_ASSERT_EQUAL_INT_MESSAGE(CELLULAR_STATUS_SEARCHING, StatusIndicator_GetCellular(),
        "El estado deberia seguir siendo SEARCHING");
    TEST_PASS_MESSAGE("Confirmar visualmente el patron 200ms ON / 1800ms OFF");
}

/*==================[TEST-MW-04]===============================================*/
TEST_CASE("TEST-MW-04 READY pattern: LED_QUECTEL 1800ms ON / 200ms OFF", "[status_indicator][blink]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-04 READY PATTERN\n");
    printf("========================================\n");
    printf(">>> OBSERVE LED_QUECTEL: casi siempre prendido, <<<\n");
    printf(">>> con un apagon breve cada ~2 segundos <<<\n");

    StatusIndicator_Init();
    StatusIndicator_SetCellular(CELLULAR_STATUS_READY);

    run_status_indicator_for(8000);

    TEST_ASSERT_EQUAL_INT_MESSAGE(CELLULAR_STATUS_READY, StatusIndicator_GetCellular(),
        "El estado deberia seguir siendo READY");
    TEST_PASS_MESSAGE("Confirmar visualmente el patron 1800ms ON / 200ms OFF");
}

/*==================[TEST-MW-05]===============================================*/
TEST_CASE("TEST-MW-05 TRANSMITTING pattern: LED_QUECTEL 125ms ON / 125ms OFF", "[status_indicator][blink]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-05 TRANSMITTING PATTERN\n");
    printf("========================================\n");
    printf(">>> OBSERVE LED_QUECTEL: parpadeo rapido y parejo <<<\n");
    printf(">>> (aprox. 4 parpadeos por segundo) <<<\n");

    StatusIndicator_Init();
    StatusIndicator_SetCellular(CELLULAR_STATUS_TRANSMITTING);

    run_status_indicator_for(5000);

    TEST_ASSERT_EQUAL_INT_MESSAGE(CELLULAR_STATUS_TRANSMITTING, StatusIndicator_GetCellular(),
        "El estado deberia seguir siendo TRANSMITTING");
    TEST_PASS_MESSAGE("Confirmar visualmente el patron 125ms ON / 125ms OFF");
}

/*==================[TEST-MW-06]===============================================*/
TEST_CASE("TEST-MW-06 State transitions restart the blink phase immediately", "[status_indicator][blink]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-06 TRANSICION ENTRE ESTADOS\n");
    printf("========================================\n");

    StatusIndicator_Init();

    printf(">>> Arrancando en SEARCHING (destello corto, pausa larga) <<<\n");
    StatusIndicator_SetCellular(CELLULAR_STATUS_SEARCHING);
    run_status_indicator_for(14000);

    printf(">>> Cambiando a TRANSMITTING: deberia notarse el cambio <<<\n");
    printf(">>> a parpadeo rapido de forma INMEDIATA, sin arrastrar <<<\n");
    printf(">>> el tiempo pendiente del patron anterior <<<\n");
    StatusIndicator_SetCellular(CELLULAR_STATUS_TRANSMITTING);
    run_status_indicator_for(14000);

    TEST_ASSERT_EQUAL_INT_MESSAGE(CELLULAR_STATUS_TRANSMITTING, StatusIndicator_GetCellular(),
        "El estado final deberia ser TRANSMITTING");

    StatusIndicator_SetCellular(CELLULAR_STATUS_OFF); /* dejar el LED apagado al salir */

    TEST_PASS_MESSAGE("Confirmar visualmente que el cambio de patron fue inmediato");
}

/*==================[end of file]============================================*/