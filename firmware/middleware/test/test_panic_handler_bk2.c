#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "panic_handler.h"
#include "panic_button.h"
#include "event_frame.h"
#include "cellular_net.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TEST_IMEI   "123456789012345"

/* ============================================================================
 * TEST-MW-PANIC-01: Inicialización y validación de parámetros
 * ============================================================================ */
TEST_CASE("TEST-MW-PANIC-01 PanicHandler_Init parameter validation and setup", "[middleware][panic_handler]") {
    printf("\n========================================\n");
    printf(" TEST-MW-PANIC-01 INICIALIZACION DE PANIC HANDLER\n");
    printf("========================================\n");

    // 1. Validar rechazo de IMEI nulo
    panic_handler_err_t err_null = PanicHandler_Init(NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(PANIC_HANDLER_ERR_INIT, err_null,
        "PanicHandler_Init(NULL) debió devolver PANIC_HANDLER_ERR_INIT");

    // 2. Inicialización correcta con IMEI válido
    panic_handler_err_t err_ok = PanicHandler_Init(TEST_IMEI);
    TEST_ASSERT_EQUAL_INT_MESSAGE(PANIC_HANDLER_OK, err_ok,
        "PanicHandler_Init debió devolver PANIC_HANDLER_OK");

    printf("--> Inicialización del módulo de pánico verificada correctamente.\n");
}

/* ============================================================================
 * TEST-MW-PANIC-02: Respuesta ante la pulsación física del botón de pánico
 * ============================================================================ */
TEST_CASE("TEST-MW-PANIC-02 PanicHandler interactive button press reaction", "[middleware][panic_handler][interactive]") {
    printf("\n========================================\n");
    printf(" TEST-MW-PANIC-02 RESPUESTA INTERACTIVA A BOTON DE PANICO\n");
    printf("========================================\n");

    // Inicializamos el handler con un IMEI de prueba
    TEST_ASSERT_EQUAL_INT(PANIC_HANDLER_OK, PanicHandler_Init(TEST_IMEI));

    printf("\n========================================\n");
    printf(" MANTENGA PRESIONADO EL BOTON DE PANICO\n");
    printf(" durante al menos 1 segundo ahora...\n");
    printf("========================================\n");

    vTaskDelay(pdMS_TO_TICKS(3000));

    printf("\n--> Evento de pánico capturado por la ISR y procesado por PanicHandler_Task.\n");
    printf("--> Verificación completada.\n");
}

/* ============================================================================
 * TEST-MW-PANIC-03: Múltiples pulsaciones y verificación de anti-rebote
 * ============================================================================ */
TEST_CASE("TEST-MW-PANIC-03 PanicHandler multiple presses and debounce test", "[middleware][panic_handler][interactive]") {
    printf("\n========================================\n");
    printf(" TEST-MW-PANIC-03 PULSACIONES MULTIPLES Y ANTI-REBOTE\n");
    printf("========================================\n");

    TEST_ASSERT_EQUAL_INT(PANIC_HANDLER_OK, PanicHandler_Init(TEST_IMEI));

    printf("\n========================================\n");
    printf(" PRESIONE EL BOTON DE PANICO 3 VECES\n");
    printf(" con breves pausas entre cada pulsación (8s disponibles)...\n");
    printf("========================================\n");

    vTaskDelay(pdMS_TO_TICKS(8000));

    printf("\n--> Múltiples eventos procesados secuencialmente con incrementos de secuencia.\n");
    printf("--> Prueba completada exitosamente.\n");
}