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


/*==================[visual demonstration]==================================*/

TEST_CASE("Status indicator visual demonstration",
          "[status_indicator][demo]")
{
    TEST_ASSERT_TRUE(StatusIndicatorInit());

    printf("\n");
    printf("========================================\n");
    printf(" DEMOSTRACION VISUAL STATUS INDICATOR\n");
    printf("========================================\n");


    /*----------------------------------------------------------
     * 1. MODULO APAGADO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[1] MODULO APAGADO\n");
    printf("    GPIO5 = OFF\n");
    printf("    GPIO4 = OFF\n");

    StatusIndicatorSetPanic(false);
    StatusIndicatorSetCellular(CELLULAR_STATUS_OFF);

    vTaskDelay(pdMS_TO_TICKS(5000));


    /*----------------------------------------------------------
     * 2. MODULO ARRANCANDO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[2] MODULO ARRANCANDO\n");
    printf("    GPIO5 = ON FIJO\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_STARTING);

    vTaskDelay(pdMS_TO_TICKS(5000));


    /*----------------------------------------------------------
     * 3. BUSCANDO RED
     *----------------------------------------------------------*/

    printf("\n");
    printf("[3] BUSCANDO RED\n");
    printf("    GPIO5 = 200 ms ON / 1800 ms OFF\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_SEARCHING);

    for (int i = 0; i < 200; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*----------------------------------------------------------
     * 4. REGISTRADO EN RED
     *----------------------------------------------------------*/

    printf("\n");
    printf("[4] REGISTRADO EN RED\n");
    printf("    GPIO5 = 1800 ms ON / 200 ms OFF\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);

    for (int i = 0; i < 200; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*----------------------------------------------------------
     * 5. TRANSMITIENDO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[5] TRANSMITIENDO DATOS\n");
    printf("    GPIO5 = 125 ms ON / 125 ms OFF\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_TRANSMITTING);

    for (int i = 0; i < 240; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*----------------------------------------------------------
     * 6. VUELTA A READY
     *----------------------------------------------------------*/

    printf("\n");
    printf("[6] TRANSMISION FINALIZADA\n");
    printf("    GPIO5 vuelve a estado READY\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);

    for (int i = 0; i < 200; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*----------------------------------------------------------
     * 7. ALARMA DE PANICO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[7] ALARMA DE PANICO\n");
    printf("    GPIO4 = ON\n");
    printf("    GPIO5 = READY\n");

    StatusIndicatorSetPanic(true);
    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);

    for (int i = 0; i < 240; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*----------------------------------------------------------
     * 8. TRANSMITIENDO ALARMA
     *----------------------------------------------------------*/

    printf("\n");
    printf("[8] TRANSMITIENDO ALARMA\n");
    printf("    GPIO4 = ON\n");
    printf("    GPIO5 = TRANSMITIENDO\n");

    StatusIndicatorSetCellular(CELLULAR_STATUS_TRANSMITTING);

    for (int i = 0; i < 240; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*----------------------------------------------------------
     * 9. SERVIDOR CONFIRMA RECEPCION
     *----------------------------------------------------------*/

    printf("\n");
    printf("[9] SERVIDOR CONFIRMA RECEPCION\n");
    printf("    GPIO4 = OFF\n");
    printf("    GPIO5 = READY\n");

    StatusIndicatorSetPanic(false);
    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);

    for (int i = 0; i < 240; i++)
    {
        StatusIndicatorRunStep();
        vTaskDelay(pdMS_TO_TICKS(25));
    }


    /*----------------------------------------------------------
     * 10. MODULO APAGADO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[10] MODULO APAGADO\n");
    printf("     GPIO4 = OFF\n");
    printf("     GPIO5 = OFF\n");

    StatusIndicatorSetPanic(false);
    StatusIndicatorSetCellular(CELLULAR_STATUS_OFF);

    vTaskDelay(pdMS_TO_TICKS(15000));


    printf("\n");
    printf("========================================\n");
    printf(" FIN DE DEMOSTRACION\n");
    printf("========================================\n");

    TEST_ASSERT_TRUE(true);
}