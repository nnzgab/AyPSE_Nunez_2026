#include "unity.h"
#include "gpio_hal.h"
#include "board_config.h"
/* FreeRTOS: incluir antes de usar pdMS_TO_TICKS y vTaskDelay */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <stdbool.h>

#define DELAY_TIME_MS 100
#define TOGGLE_COUNT 10

#define INTERRUPT_TEST_DELAY_MS    100


///////////////////////////////////////
// para test de la interrupcion hal

/* Variable modificada por el callback */
static volatile int gpio_interrupt_count = 0;

/*
 * Callback utilizado por el test.
 *
 * Esta función será ejecutada cuando ocurra
 * la interrupción del GPIO.
 */
static void test_gpio_callback(void *args)
{
    gpio_interrupt_count++;
}
//////////////////////////////////////

/**
 * @brief Verifica que el GPIO4 utilizado por el LED de estado
 *        pueda configurarse como salida y cambiar entre HIGH y LOW.
 */

 TEST_CASE("TEST-01 GPIOInit configures pin as output without crash", "[drivers_hal][gpio]") {
    /* Inicialización repetida para probar robustez */
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));

    /* Si llegamos hasta aquí sin reset/excepción, el test pasa */
    TEST_ASSERT_TRUE_MESSAGE(true, "Init ejecutado sin crash");
}


TEST_CASE("TEST-02 GPIOOn/Off panic_led", "[gpio][panic][on_off]") {
    /* Encender y verificar HIGH */
    GPIOOn(GPIO_PANIC_LED_STATUS);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS)); /* tiempo para estabilizar la salida */
    TEST_ASSERT_TRUE_MESSAGE(GPIORead(GPIO_PANIC_LED_STATUS) == 1, "GPIORead no reporta HIGH tras GPIOOn");

    /* Apagar y verificar LOW */
    GPIOOff(GPIO_PANIC_LED_STATUS);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_TRUE_MESSAGE(GPIORead(GPIO_PANIC_LED_STATUS) == 0, "GPIORead no reporta LOW tras GPIOOff");
}

TEST_CASE("TEST-03 GPIOToggle alternates state visibly", "[gpio][panic][toogle]") {
    /* Aseguramos estado inicial conocido */
    GPIOOff(GPIO_PANIC_LED_STATUS);
    vTaskDelay(DELAY_TIME_MS);

    for (int i = 0; i < TOGGLE_COUNT; ++i) {
        GPIOToggle(GPIO_PANIC_LED_STATUS);
        vTaskDelay(DELAY_TIME_MS);

        /* Comprobación interna rápida: GPIORead debe coincidir con pin_state esperado */
        int read = GPIORead(GPIO_PANIC_LED_STATUS);

        /* calcular el valor esperado si partimos en LOW */
        int expected = (i % 2 == 0) ? 1 : 0;

        /* Alternancia esperada: si i es par, primer toggle -> HIGH, etc. */
        TEST_ASSERT_EQUAL_MESSAGE(expected, read, "GPIOToggle no produjo la alternancia esperada");
    }
}


TEST_CASE("TEST-04 GPIOState drives HIGH and LOW measurable", "[gpio][State]") {
   
    /* Set HIGH and verify */
    GPIOState(GPIO_PANIC_LED_STATUS, true);
    vTaskDelay(DELAY_TIME_MS);
    TEST_ASSERT_TRUE_MESSAGE(GPIORead(GPIO_PANIC_LED_STATUS) == 1, "GPIOState(true) no dejó HIGH");

    /* Measure with multímetro and document (manual step) */

    /* Set LOW and verify */
    GPIOState(GPIO_PANIC_LED_STATUS, false);
    vTaskDelay(DELAY_TIME_MS);
    TEST_ASSERT_TRUE_MESSAGE(GPIORead(GPIO_PANIC_LED_STATUS) == 0, "GPIOState(false) no dejó LOW");
}

TEST_CASE("TEST-05 GPIO23 input pull up reads correctly", "[gpio][input]") {
    GPIOInit(23, GPIO_INPUT); // o activar pull up en board config
    vTaskDelay(pdMS_TO_TICKS(50*DELAY_TIME_MS));
    printf(" pulsa el boton\n");
    vTaskDelay(pdMS_TO_TICKS(50*DELAY_TIME_MS));
    // Paso manual: conectar GPIO23 a GND y verificar lectura 0
    //vTaskDelay(DELAY_TIME_MS);
    int read_low = GPIORead(23);
    vTaskDelay(pdMS_TO_TICKS(50*DELAY_TIME_MS));
    
     
    TEST_ASSERT_EQUAL_MESSAGE(0, read_low, "GPIO23 no leyo LOW cuando estaba a GND");

    printf(" solta el boton\n");
    vTaskDelay(pdMS_TO_TICKS(50*DELAY_TIME_MS));

    // Paso manual 2: dejar abierto (pull up) o conectar a 3.3V
    //vTaskDelay(5*DELAY_TIME_MS);
    int read_high = GPIORead(23);
    TEST_ASSERT_EQUAL_MESSAGE(1, read_high, "GPIO23 no leyo HIGH cuando estaba en pull up");
}


TEST_CASE("TEST-06 GPIO interrupt detects falling edge and deactivation", "[gpio][interrupt]")
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-06 GPIO INTERRUPT\n");
    printf("========================================\n");

    // 1. Inicializamos GPIO23 como entrada (¡Descomentado!).
    // GPIO23 tiene pull-up interno: sin pulsar -> HIGH, Pulsado -> LOW
    GPIOInit(GPIO_PANIC_BTN, GPIO_INPUT);

    // Damos un tiempo breve para estabilizar el hardware
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Reiniciamos el contador antes del test.
    gpio_interrupt_count = 0;

    // Activamos interrupción por flanco descendente.
    GPIOActivInt(
        GPIO_PANIC_BTN,
        GPIO_INT_FALLING,
        test_gpio_callback,
        NULL
    );

    printf("\n========================================\n");
    printf(" FASE 1: PRUEBA DE ACTIVACION\n");
    printf(" GPIO23 debe estar HIGH.\n");
    printf(" PRESIONAR EL BOTON AHORA.\n");
    printf("========================================\n");

    // Esperamos 5 segundos a que el usuario presione el botón.
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Desactivamos la interrupción inmediatamente después de la espera.
    GPIODeactivInt(GPIO_PANIC_BTN);

    // Validamos la FASE 1
    printf("\nCantidad de interrupciones detectadas (Fase 1): %d\n", gpio_interrupt_count);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, gpio_interrupt_count, "No se detecto ninguna interrupcion al presionar el boton");


    // --- FASE 2: PRUEBA DE DESACTIVACION ---
    
    // Guardamos el contador actual. Si la interrupción se apagó bien, 
    // este número ya no debería cambiar.
    int conteo_fase1 = gpio_interrupt_count;

    printf("\n========================================\n");
    printf(" FASE 2: PRUEBA DE DESACTIVACION\n");
    printf(" PRESIONAR EL BOTON NUEVAMENTE.\n");
    printf(" El contador NO deberia subir.\n");
    printf("========================================\n");

    // Damos otros 5 segundos para que vuelvas a presionar el botón físico
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Validamos la FASE 2
    printf("Cantidad de interrupciones detectadas (Fase 2): %d\n", gpio_interrupt_count);
    TEST_ASSERT_EQUAL_INT_MESSAGE(conteo_fase1, gpio_interrupt_count, "Error: La interrupcion sumó a pesar de haber sido desactivada con GPIODeactivInt");
}


/*
TEST_CASE("GPIO4 controls panic LED", "[drivers_hal][gpio]")
{
    printf("\n========================================\n");
    printf(" GPIO4 - HAL READ/WRITE TEST\n");
    printf("========================================\n");

    printf("Inicializo GPIO4 como salida\n");
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);

    printf("Enciendo GPIO4\n");
    GPIOOn(GPIO_PANIC_LED_STATUS);
    vTaskDelay(pdMS_TO_TICKS(5000));

    bool state_high = GPIORead(GPIO_4);
    printf("GPIO4 Readback (expected 1): %d\n", state_high);

    TEST_ASSERT_MESSAGE( state_high == true  , "GPIO4 no se puso en alto");

    TEST_ASSERT_TRUE(GPIORead(GPIO_PANIC_LED_STATUS) == 1);

    printf("Apago GPIO4\n");
    GPIOOff(GPIO_PANIC_LED_STATUS);
    vTaskDelay(pdMS_TO_TICKS(5000));

    TEST_ASSERT_TRUE(GPIORead(GPIO_PANIC_LED_STATUS) == 0);

    printf("Toggleo GPIO4 se deberia encender\n");
    GPIOToggle(GPIO_PANIC_LED_STATUS);
    TEST_ASSERT_TRUE(GPIORead(GPIO_PANIC_LED_STATUS) == 1);
    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("Toggleo nuevamente GPIO4 se deberia apagar\n");
    GPIOToggle(GPIO_PANIC_LED_STATUS);
    TEST_ASSERT_TRUE(GPIORead(GPIO_PANIC_LED_STATUS) == 0);
    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("Establesco GPIO4 en Alto se deberia encender\n");
    GPIOState(GPIO_PANIC_LED_STATUS, true);
    TEST_ASSERT_TRUE(GPIORead(GPIO_PANIC_LED_STATUS) == 1);
    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("Establesco GPIO4 en Bajo se deberia apagar\n");
    GPIOState(GPIO_PANIC_LED_STATUS, false);
    TEST_ASSERT_TRUE(GPIORead(GPIO_PANIC_LED_STATUS) == 0);
    vTaskDelay(pdMS_TO_TICKS(5000));


    
}

*/
/*
TEST_CASE("GPIO23 controls panic BUTTON", "[drivers_hal][gpio]")
{
    printf("Inicializo GPIO23 como entrada\n");
    GPIOInit(GPIO_PANIC_BTN, GPIO_INPUT);

    printf("Colocar GPIO23 a GND\n");
    GPIORead(GPIO_PANIC_BTN)

    */

    TEST_CASE(
    "TEST-07 GPIO6 QUECTEL PWRKEY output",
    "[gpio][powerkey]"
)
{
    printf("\n");
    printf("=====================================\n");
    printf(" GPIO6 - QUECTEL PWRKEY TEST\n");
    printf("=====================================\n");

    /*
     * GPIO6 es utilizado por el BSP como PWRKEY
     * del módulo Quectel.
     *
     * En este nivel solamente verificamos
     * el comportamiento del GPIO.
     */

    GPIOInit(
        QUECTEL_PWRKEY_PIN,
        GPIO_OUTPUT
    );

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    /*
     * Estado inactivo:
     * GPIO6 = LOW
     */
    GPIOOff(
        QUECTEL_PWRKEY_PIN
    );

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    TEST_ASSERT_EQUAL_MESSAGE(
        0,
        GPIORead(QUECTEL_PWRKEY_PIN),
        "GPIO6 no quedo en LOW"
    );

    /*
     * Activar PWRKEY:
     * GPIO6 = HIGH
     */
    GPIOOn(
        QUECTEL_PWRKEY_PIN
    );

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    TEST_ASSERT_EQUAL_MESSAGE(
        1,
        GPIORead(QUECTEL_PWRKEY_PIN),
        "GPIO6 no quedo en HIGH"
    );

    /*
     * Mantener HIGH durante el tiempo
     * requerido por la prueba física.
     *
     * Esto NO verifica al Quectel.
     * Solamente permite observar/medir GPIO6.
     */
    vTaskDelay(
        pdMS_TO_TICKS(2100)
    );

    /*
     * Liberar PWRKEY.
     */
    GPIOOff(
        QUECTEL_PWRKEY_PIN
    );

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    TEST_ASSERT_EQUAL_MESSAGE(
        0,
        GPIORead(QUECTEL_PWRKEY_PIN),
        "GPIO6 no regreso a LOW"
    );

    printf("PWRKEY GPIO6: pulso generado correctamente.\n");
}