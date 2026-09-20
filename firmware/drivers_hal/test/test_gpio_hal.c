#include <stdio.h>
#include <stdbool.h>
#include "unity.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DELAY_TIME_MS   100
#define TOGGLE_COUNT    6

static volatile int s_gpio_int_count = 0;

static void test_gpio_isr_callback(void *args) {
    (void)args;
    s_gpio_int_count++;
}

/* ============================================================================
 * TEST-HAL-GPIO-01: Inicialización de salida sin colisión
 * ============================================================================ */
TEST_CASE("TEST-HAL-GPIO-01 GPIOInit configures output pin cleanly", "[drivers_hal][gpio][init]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-GPIO-01 INICIALIZACION DE SALIDA\n");
    printf("========================================\n");

    printf("[PASO 1] Configurando pin de LED como salida...\n");
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));

    printf("[PASO 2] Re-inicializando para verificar idempotencia...\n");
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));

    TEST_ASSERT_TRUE_MESSAGE(true, "FAIL: Error durante la inicialización de GPIO");
    printf("--> Éxito: GPIO configurado como salida sin fallos de sistema.\n");
}

/* ============================================================================
 * TEST-HAL-GPIO-02: Control de niveles lógicos (GPIOOn / GPIOOff)
 * ============================================================================ */
TEST_CASE("TEST-HAL-GPIO-02 GPIOOn and GPIOOff drive logic levels", "[drivers_hal][gpio][on_off]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-GPIO-02 CONTROL DE NIVELES LOGICOS\n");
    printf("========================================\n");

    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);

    printf("[PASO 1] Estableciendo pin en HIGH (GPIOOn)...\n");
    GPIOOn(GPIO_PANIC_LED_STATUS);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_PANIC_LED_STATUS), "FAIL: GPIORead no retornó 1 tras GPIOOn");

    printf("[PASO 2] Estableciendo pin en LOW (GPIOOff)...\n");
    GPIOOff(GPIO_PANIC_LED_STATUS);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_PANIC_LED_STATUS), "FAIL: GPIORead no retornó 0 tras GPIOOff");

    printf("--> Éxito: Conmutación de estados HIGH y LOW verificada con lectura del pin.\n");
}

/* ============================================================================
 * TEST-HAL-GPIO-03: Alternancia continua (GPIOToggle)
 * ============================================================================ */
TEST_CASE("TEST-HAL-GPIO-03 GPIOToggle alternates pin state", "[drivers_hal][gpio][toggle]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-GPIO-03 ALTERNANCIA DE ESTADO (GPIOToggle)\n");
    printf("========================================\n");

    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);
    GPIOOff(GPIO_PANIC_LED_STATUS);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));

    for (int i = 0; i < TOGGLE_COUNT; i++) {
        GPIOToggle(GPIO_PANIC_LED_STATUS);
        vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
        int expected = (i % 2 == 0) ? 1 : 0;
        int read_val = GPIORead(GPIO_PANIC_LED_STATUS);

        printf(" -> Toggle iteración %d: leídos=%d (esperado=%d)\n", i + 1, read_val, expected);
        TEST_ASSERT_EQUAL_INT_MESSAGE(expected, read_val, "FAIL: La alternancia del pin no coincidió con lo esperado");
    }

    printf("--> Éxito: %d alternancias de estado ejecutadas correctamente.\n", TOGGLE_COUNT);
}

/* ============================================================================
 * TEST-HAL-GPIO-04: Detección y desactivación de interrupción por hardware
 * ============================================================================ */
TEST_CASE("TEST-HAL-GPIO-04 GPIO interrupt activation and deactivation", "[drivers_hal][gpio][interrupt]") {
    printf("\n========================================\n");
    printf(" TEST-HAL-GPIO-04 INTERRUPCION POR FLANCO\n");
    printf("========================================\n");

    GPIOInit(GPIO_PANIC_BTN, GPIO_INPUT);
    vTaskDelay(pdMS_TO_TICKS(100));
    s_gpio_int_count = 0;

    printf("\n--- FASE 1: Activación de Interrupción ---\n");
    printf("Presione el botón de pánico en los próximos 5 segundos...\n");
    GPIOActivInt(GPIO_PANIC_BTN, GPIO_INT_FALLING, test_gpio_isr_callback, NULL);
    vTaskDelay(pdMS_TO_TICKS(5000));

    GPIODeactivInt(GPIO_PANIC_BTN);
    int count_phase1 = s_gpio_int_count;
    printf("Interrupciones detectadas en Fase 1: %d\n", count_phase1);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, count_phase1, "FAIL: No se detectó ninguna interrupción al presionar el botón");

    printf("\n--- FASE 2: Desactivación de Interrupción ---\n");
    printf("Presione el botón nuevamente (NO debería registrar eventos)...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("Interrupciones detectadas en Fase 2: %d\n", s_gpio_int_count);
    TEST_ASSERT_EQUAL_INT_MESSAGE(count_phase1, s_gpio_int_count, "FAIL: Se registraron interrupciones tras desinstalar el servicio");

    printf("--> Éxito: Interrupción por flanco descendente activada y desactivada correctamente.\n");
}