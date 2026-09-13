#include <stdio.h>

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "panic_button.h"

/*==================[internal data definition]=================================*/
static volatile int panic_interrupt_count = 0;

/*==================[internal functions definition]==============================*/
static void test_panic_button_callback(void *arg)
{
    panic_interrupt_count++;
}

/*==================[TEST-BSP-01]===============================================*/
TEST_CASE("TEST-BSP-01 PanicButtonInit initializes GPIO and time base", "[panic_button][init]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-BSP-01 PANIC BUTTON INIT\n");
    printf("========================================\n");

    bool ok = PanicButtonInit();
    TEST_ASSERT_TRUE_MESSAGE(ok, "PanicButtonInit deberia devolver true");
}

/*==================[TEST-BSP-02]===============================================*/
TEST_CASE("TEST-BSP-02 PanicButtonIsPressed reflects physical button state", "[panic_button][state]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-BSP-02 PANIC BUTTON STATE\n");
    printf("========================================\n");

    PanicButtonInit();

    printf("\n========================================\n");
    printf(" NO presione el boton (reposo).\n");
    printf(" Verificando en 3 segundos...\n");
    printf("========================================\n");
    vTaskDelay(pdMS_TO_TICKS(3000));

    TEST_ASSERT_FALSE_MESSAGE(PanicButtonIsPressed(),
        "El boton deberia leerse como NO presionado en reposo");

    printf("\n========================================\n");
    printf(" MANTENGA PRESIONADO EL BOTON AHORA.\n");
    printf(" Verificando en 3 segundos...\n");
    printf("========================================\n");
    vTaskDelay(pdMS_TO_TICKS(3000));

    TEST_ASSERT_TRUE_MESSAGE(PanicButtonIsPressed(),
        "El boton deberia leerse como presionado mientras se lo mantiene apretado");

    printf("\nSUELTE EL BOTON.\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

/*==================[TEST-BSP-03]===============================================*/
TEST_CASE("TEST-BSP-03 PanicButtonAttachInterrupt detects a single press filtering bounces", "[panic_button][interrupt]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-BSP-03 PANIC BUTTON INTERRUPT\n");
    printf("========================================\n");

    PanicButtonInit();
    panic_interrupt_count = 0;

    PanicButtonAttachInterrupt(test_panic_button_callback, NULL);

    printf("\n========================================\n");
    printf(" PRESIONE EL BOTON UNA SOLA VEZ, BREVEMENTE.\n");
    printf(" Se espera exactamente 1 evento (el debounce\n");
    printf(" de 30 ms debe filtrar los rebotes mecanicos).\n");
    printf("========================================\n");

    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("\nCantidad de eventos detectados: %d (esperado: 1)\n", panic_interrupt_count);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, panic_interrupt_count,
        "Se esperaba exactamente 1 evento por una unica pulsacion (revisar debounce)");
}

/*==================[TEST-BSP-04]===============================================*/
TEST_CASE("TEST-BSP-04 PanicButtonAttachInterrupt counts multiple distinct presses", "[panic_button][interrupt]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-BSP-04 PANIC BUTTON MULTIPLE PRESSES\n");
    printf("========================================\n");

    PanicButtonInit();
    panic_interrupt_count = 0;

    PanicButtonAttachInterrupt(test_panic_button_callback, NULL);

    printf("\n========================================\n");
    printf(" PRESIONE EL BOTON 3 VECES, con al menos\n");
    printf(" medio segundo de diferencia entre cada una.\n");
    printf(" Tiene 8 segundos.\n");
    printf("========================================\n");

    vTaskDelay(pdMS_TO_TICKS(8000));

    printf("\nCantidad de eventos detectados: %d (esperado: 3)\n", panic_interrupt_count);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, panic_interrupt_count,
        "La cantidad de eventos no coincide con la cantidad de pulsaciones realizadas");

    /* Dejamos la interrupcion apagada para no interferir con otros tests */
    PanicButtonDetachInterrupt();

}

/*==================[end of file]============================================*/

