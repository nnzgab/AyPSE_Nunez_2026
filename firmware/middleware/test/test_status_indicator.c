#include "unity.h"

#include "status_indicator.h"
#include "gpio_hal.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*==================[tests]==================================================*/

TEST_CASE("StatusIndicatorInit turns LEDs off", "[status_indicator]")
{
    TEST_ASSERT_TRUE(StatusIndicatorInit());

    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_QUECTEL_LED_STATUS));
}


TEST_CASE("Panic indicator turns LED on", "[status_indicator]")
{
    TEST_ASSERT_TRUE(StatusIndicatorInit());

    StatusIndicatorSetPanic(true);

    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_PANIC_LED_STATUS));
}


TEST_CASE("Panic indicator turns LED off", "[status_indicator]")
{
    TEST_ASSERT_TRUE(StatusIndicatorInit());

    StatusIndicatorSetPanic(true);
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_PANIC_LED_STATUS));

    StatusIndicatorSetPanic(false);

    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));
}


TEST_CASE("Cellular OFF turns LED off", "[status_indicator]")
{
    TEST_ASSERT_TRUE(StatusIndicatorInit());

    StatusIndicatorSetCellular(CELLULAR_STATUS_OFF);

    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_QUECTEL_LED_STATUS));
}


TEST_CASE("Cellular STARTING turns LED on", "[status_indicator]")
{
    TEST_ASSERT_TRUE(StatusIndicatorInit());

    StatusIndicatorSetCellular(CELLULAR_STATUS_STARTING);

    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_QUECTEL_LED_STATUS));
}

TEST_CASE("Status indicator visual demonstration", "[status_indicator][demo]")
{
    TEST_ASSERT_TRUE(StatusIndicatorInit());

    /*
     * ============================================================
     * 1. SISTEMA INICIALIZADO
     * ============================================================
     */

    printf("\n");
    printf("========================================\n");
    printf(" DEMOSTRACION VISUAL STATUS INDICATOR\n");
    printf("========================================\n");

    printf("\n[1] Modulo apagado\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_OFF);

    vTaskDelay(pdMS_TO_TICKS(2000));


    /*
     * ============================================================
     * 2. MODULO ARRANCANDO
     * ============================================================
     */

    printf("\n[2] Modulo arrancando - LED fijo\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_STARTING);

    vTaskDelay(pdMS_TO_TICKS(3000));


    /*
     * ============================================================
     * 3. BUSCANDO RED
     * ============================================================
     */

    printf("\n[3] Buscando red - 200 ms ON / 1800 ms OFF\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_SEARCHING);

    for (int i = 0; i < 100; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*
     * ============================================================
     * 4. REGISTRADO EN RED
     * ============================================================
     */

    printf("\n[4] Registrado en red - 1800 ms ON / 200 ms OFF\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);

    for (int i = 0; i < 100; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*
     * ============================================================
     * 5. TRANSMITIENDO
     * ============================================================
     */

    printf("\n[5] Transmitiendo - parpadeo rapido\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_TRANSMITTING);

    for (int i = 0; i < 80; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*
     * ============================================================
     * 6. VUELVE A ESTADO READY
     * ============================================================
     */

    printf("\n[6] Transmision terminada - vuelve a READY\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);

    for (int i = 0; i < 80; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*
     * ============================================================
     * 7. ALARMA
     * ============================================================
     */

    printf("\n[7] ALARMA DE PANICO\n");

    StatusIndicatorSetPanic(true);

    /*
     * Mantenemos la alarma activa mientras
     * el indicador celular sigue funcionando.
     */
    for (int i = 0; i < 80; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*
     * ============================================================
     * 8. TRANSMISION DE LA ALARMA
     * ============================================================
     */

    printf("\n[8] Transmitiendo alarma\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_TRANSMITTING);

    for (int i = 0; i < 80; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*
     * ============================================================
     * 9. SERVIDOR CONFIRMA RECEPCION
     * ============================================================
     */

    printf("\n[9] Servidor confirma alarma - alarma finalizada\n");

    StatusIndicatorSetPanic(false);

    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);

    for (int i = 0; i < 80; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*
     * ============================================================
     * 10. MODULO APAGADO
     * ============================================================
     */

    printf("\n[10] Modulo apagado\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_OFF);

    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n========================================\n");
    printf(" FIN DE DEMOSTRACION\n");
    printf("========================================\n");

    TEST_ASSERT_TRUE(true);
}