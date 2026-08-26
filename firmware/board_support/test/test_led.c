#include "unity.h"

#include "led.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define DELAY_TIME_MS    100


TEST_CASE("TEST-BSP-LED-01 LedInit initializes LEDs", "[bsp][led]")
{
    bool result;
    result = LedInit();
    TEST_ASSERT_TRUE_MESSAGE(result, "LedInit() fallo");
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
}


TEST_CASE("TEST-BSP-LED-02 LedOn turns panic LED on", "[bsp][led][panic]")
{
    LedInit();
    LedOn(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_PANIC_LED_STATUS));
}


TEST_CASE("TEST-BSP-LED-03 LedOff turns panic LED off", "[bsp][led][panic]")
{
    LedInit();
    LedOff(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));
}


TEST_CASE("TEST-BSP-LED-04 LedToggle changes panic LED state", "[bsp][led][panic]")
{
    LedInit();
    LedOff(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));

    LedToggle(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_PANIC_LED_STATUS));

    LedToggle(LED_PANIC);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));
}


TEST_CASE("TEST-BSP-LED-05 LedState controls panic LED", "[bsp][led][panic]")
{
    LedInit();
    LedState(LED_PANIC, true);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(1, GPIORead(GPIO_PANIC_LED_STATUS));

    LedState(LED_PANIC, false);
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME_MS));
    TEST_ASSERT_EQUAL(0, GPIORead(GPIO_PANIC_LED_STATUS));
}