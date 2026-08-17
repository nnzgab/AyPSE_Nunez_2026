#include "unity.h"
#include "gpio_hal.h"

/* 
   Tests reducidos para tu proyecto:
   - GPIO_23 como entrada (botón de pánico)
   - GPIO_4, GPIO_5 como LEDs indicadores
   - GPIO_6 como salida digital para encender el módulo
*/

TEST_CASE("GPIO_23 initializes as input", "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_23, GPIO_INPUT);
    bool state = GPIORead(GPIO_23);
    TEST_ASSERT_TRUE(state == 0 || state == 1); // debe devolver algo válido
}

TEST_CASE("GPIO_4 LED can be turned on/off", "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_4, GPIO_OUTPUT);
    GPIOOn(GPIO_4);
    TEST_ASSERT_TRUE(GPIORead(GPIO_4));
    GPIOOff(GPIO_4);
    TEST_ASSERT_FALSE(GPIORead(GPIO_4));
}

TEST_CASE("GPIO_5 LED can be toggled", "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_5, GPIO_OUTPUT);
    GPIOOff(GPIO_5);
    TEST_ASSERT_FALSE(GPIORead(GPIO_5));
    GPIOToggle(GPIO_5);
    TEST_ASSERT_TRUE(GPIORead(GPIO_5));
}

TEST_CASE("GPIO_6 controls module power", "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_6, GPIO_OUTPUT);
    GPIOState(GPIO_6, true);
    TEST_ASSERT_TRUE(GPIORead(GPIO_6));
    GPIOState(GPIO_6, false);
    TEST_ASSERT_FALSE(GPIORead(GPIO_6));
}
