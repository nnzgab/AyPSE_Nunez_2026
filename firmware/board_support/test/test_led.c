/*

#include "unity.h"

#include "led.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define DELAY_TIME_MS    500


TEST_CASE("TEST-BSP-LED-01 LedInit initializes LEDs", "[bsp][led]")
{
    bool result;
    result = LedInit();
    TEST_ASSERT_TRUE_MESSAGE(result, "LedInit() fallo");
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
}


TEST_CASE("TEST-BSP-LED-02 LedOn turns LEDs on", "[bsp][led][panic][quectel]")
{
    LedInit();
    
    LedOn(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_PANIC_LED_STATUS));

    LedOn(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_QUECTEL_LED_STATUS));
}


TEST_CASE("TEST-BSP-LED-03 LedOff turns LEDs off", "[bsp][led][panic][quectel]")
{
    LedInit();
    
    LedOff(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));

    LedOff(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_QUECTEL_LED_STATUS));
}


TEST_CASE("TEST-BSP-LED-04 LedToggle changes LED states", "[bsp][led][panic][quectel]")
{
    LedInit();
    
    //Prueba sobre LED_PANIC 
    LedOff(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));

    LedToggle(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_PANIC_LED_STATUS));

    LedToggle(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));

    //Prueba sobre LED_QUECTEL
    LedOff(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_QUECTEL_LED_STATUS));

    LedToggle(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_QUECTEL_LED_STATUS));

    LedToggle(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_QUECTEL_LED_STATUS));
}


TEST_CASE("TEST-BSP-LED-05 LedState controls LEDs", "[bsp][led][panic][quectel]")
{
    LedInit();
    
    //Control de LED_PANIC 
    LedState(LED_PANIC, true);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_PANIC_LED_STATUS));

    LedState(LED_PANIC, false);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));

    //Control de LED_QUECTEL 
    LedState(LED_QUECTEL, true);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_QUECTEL_LED_STATUS));

    LedState(LED_QUECTEL, false);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_QUECTEL_LED_STATUS));
}

*/
#include <stdio.h>
#include "unity.h"
#include "led.h"
#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DELAY_TIME_MS    1000

/* ============================================================================
 * TEST-BSP-LED-01: Inicialización
 * ============================================================================ */
TEST_CASE("TEST-BSP-LED-01 LedInit initializes LEDs", "[bsp][led]") {
    printf("\n========================================\n");
    printf(" TEST-BSP-LED-01 INICIALIZACION DE LEDS\n");
    printf("========================================\n");

    bool result = LedInit();
    TEST_ASSERT_TRUE_MESSAGE(result, "LedInit() devolvió false");
    
    printf("LEDs inicializados correctamente (estado por defecto: APAGADO).\n");
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
}

/* ============================================================================
 * TEST-BSP-LED-02: Encendido (LedOn)
 * ============================================================================ */
TEST_CASE("TEST-BSP-LED-02 LedOn turns LEDs on", "[bsp][led][panic][quectel]") {
    printf("\n========================================\n");
    printf(" TEST-BSP-LED-02 ENCENDIDO DE LEDS (LedOn)\n");
    printf("========================================\n");

    LedInit();

    printf("Encendiendo LED_PANIC (Verificar luz encendida)...\n");
    LedOn(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_PANIC_LED_STATUS), "Error en nivel logico GPIO_PANIC");

    printf("Encendiendo LED_QUECTEL (Verificar luz encendida)...\n");
    LedOn(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_QUECTEL_LED_STATUS), "Error en nivel logico GPIO_QUECTEL");
}

/* ============================================================================
 * TEST-BSP-LED-03: Apagado (LedOff)
 * ============================================================================ */
TEST_CASE("TEST-BSP-LED-03 LedOff turns LEDs off", "[bsp][led][panic][quectel]") {
    printf("\n========================================\n");
    printf(" TEST-BSP-LED-03 APAGADO DE LEDS (LedOff)\n");
    printf("========================================\n");

    LedInit();
    // Encendemos previamente para verificar que LedOff realmente apague
    LedOn(LED_PANIC);
    LedOn(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(500));

    printf("Apagando LED_PANIC...\n");
    LedOff(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_PANIC_LED_STATUS), "Error: LED_PANIC no se apago");

    printf("Apagando LED_QUECTEL...\n");
    LedOff(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_QUECTEL_LED_STATUS), "Error: LED_QUECTEL no se apago");
}

/* ============================================================================
 * TEST-BSP-LED-04: Conmutación (LedToggle)
 * ============================================================================ */
TEST_CASE("TEST-BSP-LED-04 LedToggle changes LED states", "[bsp][led][panic][quectel]") {
    printf("\n========================================\n");
    printf(" TEST-BSP-LED-04 CONMUTACION DE LEDS (LedToggle)\n");
    printf("========================================\n");

    LedInit(); // Comienzan en 0 (OFF)

    printf("Conmutando LED_PANIC (0 -> 1)...\n");
    LedToggle(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_PANIC_LED_STATUS), "Error: LedToggle no cambio a HIGH");

    printf("Conmutando LED_QUECTEL (0 -> 1)...\n");
    LedToggle(LED_QUECTEL);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_QUECTEL_LED_STATUS), "Error: LedToggle no cambio a HIGH");
}

/* ============================================================================
 * TEST-BSP-LED-05: Estado directo (LedState)
 * ============================================================================ */
TEST_CASE("TEST-BSP-LED-05 LedState controls LEDs", "[bsp][led][panic][quectel]") {
    printf("\n========================================\n");
    printf(" TEST-BSP-LED-05 CONTROL DIRECTO DE ESTADO (LedState)\n");
    printf("========================================\n");

    LedInit();

    printf("Fijando LED_PANIC en ON via LedState(..., true)...\n");
    LedState(LED_PANIC, true);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_PANIC_LED_STATUS), "Error: LedState true no activo el pin");

    printf("Fijando LED_QUECTEL en ON via LedState(..., true)...\n");
    LedState(LED_QUECTEL, true);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, GPIORead(GPIO_QUECTEL_LED_STATUS), "Error: LedState true no activo el pin");

    printf("Fijando ambos LEDs en OFF via LedState(..., false)...\n");
    LedState(LED_PANIC, false);
    LedState(LED_QUECTEL, false);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_PANIC_LED_STATUS), "Error: LedState false no apago PANIC");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, GPIORead(GPIO_QUECTEL_LED_STATUS), "Error: LedState false no apago QUECTEL");
}