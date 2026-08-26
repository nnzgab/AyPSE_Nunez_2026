#include "unity.h"

#include "panic_button.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>


#define BUTTON_WAIT_MS    100


TEST_CASE("TEST-BSP-BUTTON-01 PanicButtonInit initializes button", "[bsp][panic][button]")
{
    bool result;
    result = PanicButtonInit();
    TEST_ASSERT_TRUE_MESSAGE(result, "PanicButtonInit() fallo");
    vTaskDelay(pdMS_TO_TICKS(BUTTON_WAIT_MS));
}

TEST_CASE(
    "TEST-BSP-BUTTON-02 PanicButtonIsPressed reads button state",
    "[bsp][panic][button]"
)
{
    PanicButtonInit();

    printf("\n");
    printf("=====================================\n");
    printf(" TEST PANIC BUTTON STATE\n");
    printf("=====================================\n");

    printf("\nPresione el boton...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    TEST_ASSERT_TRUE_MESSAGE(
        PanicButtonIsPressed(),
        "El boton deberia estar presionado"
    );

    printf("Deje el boton LIBERADO...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    TEST_ASSERT_FALSE_MESSAGE(
        PanicButtonIsPressed(),
        "El boton deberia estar liberado"
    );
}



TEST_CASE(
    "TEST-BSP-BUTTON-03 PanicButton generates press event",
    "[bsp][panic][button][interrupt]"
)
{
    printf("\n");
    printf("=====================================\n");
    printf(" PANIC BUTTON INTERRUPT TEST\n");
    printf("=====================================\n");

    printf("\nInicializando boton...\n");

    PanicButtonInit();

    printf("GPIO23 = %d\n",
           GPIORead(GPIO_PANIC_BTN));

    printf("PanicButtonIsPressed = %d\n",
           PanicButtonIsPressed());

    bool initial_event = PanicButtonPressedEvent();

    printf("Evento inicial = %d\n",
           initial_event);

    TEST_ASSERT_FALSE_MESSAGE(
        initial_event,
        "Hay un evento pendiente antes de presionar el boton"
    );

    printf("\n-------------------------------------\n");
    printf("Ahora presione el boton UNA VEZ\n");
    printf("-------------------------------------\n");

    vTaskDelay(
        pdMS_TO_TICKS(5000)
    );

    bool event = PanicButtonPressedEvent();

    printf("Evento despues de esperar = %d\n", event);

    TEST_ASSERT_TRUE_MESSAGE(
        event,
        "No se detecto el evento del boton"
    );
}

TEST_CASE(
    "TEST-BSP-BUTTON-04 PanicButton debounce",
    "[bsp][panic][button][debounce]"
)
{
    PanicButtonInit();

    /* Limpiamos cualquier evento viejo por precaución (lo que vimos en el error anterior) */
    PanicButtonPressedEvent(); 

    printf("\n");
    printf("=====================================\n");
    printf(" PANIC BUTTON DEBOUNCE TEST\n");
    printf("=====================================\n");
    printf("Presione y suelte el boton (tienes 5 segundos)...\n");

    bool first_event = false;
    int wait_time_ms = 0;
    const int step_ms = 50; // Chequeamos cada 50 milisegundos

    /* 
     * POLLING CON TIMEOUT: 
     * Revisamos el botón en ciclos cortos hasta un máximo de 5000 ms (5 segundos).
     */
    while (wait_time_ms < 5000) {
        first_event = PanicButtonPressedEvent();
        
        if (first_event) {
            printf(" -> ¡Pulsación detectada a los %d ms!\n", wait_time_ms);
            break; // ¡Salimos del bucle inmediatamente!
        }
        
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        wait_time_ms += step_ms;
    }

    /* 1. Verificamos que efectivamente se haya presionado el botón */
    TEST_ASSERT_TRUE_MESSAGE(
        first_event,
        "Tiempo agotado: No se detecto la pulsacion en los 5 segundos"
    );

    /* 
     * 2. Prueba de anti-rebote (Debounce)
     * Ya detectamos el evento. Ahora esperamos un instante para permitir
     * que la mecánica física del botón termine de vibrar (los rebotes suelen durar < 50ms).
     */
    vTaskDelay(pdMS_TO_TICKS(150)); 

    bool second_event = PanicButtonPressedEvent();

    /* 3. Verificamos que el ruido mecánico no haya generado eventos falsos */
    TEST_ASSERT_FALSE_MESSAGE(
        second_event,
        "Una pulsacion genero mas de un evento (falla del algoritmo anti-rebote)"
    );
}