#include <stdio.h>

#include "unity.h"
#include "gpio_hal.h"
#include "board_config.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


TEST_CASE("GPIO4 direct ESP-IDF diagnostic",
          "[gpio]")
{
    printf("\n");
    printf("========================================\n");
    printf(" GPIO4 - DIRECT ESP-IDF TEST\n");
    printf("========================================\n");

    gpio_reset_pin(GPIO_NUM_4);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);

    printf("GPIO4 -> HIGH\n");

    gpio_set_level(GPIO_NUM_4, 1);

    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("gpio_get_level(): %d\n",
           gpio_get_level(GPIO_NUM_4));

    printf("GPIO4 -> LOW\n");

    gpio_set_level(GPIO_NUM_4, 0);

    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("gpio_get_level(): %d\n",
           gpio_get_level(GPIO_NUM_4));

    TEST_ASSERT_EQUAL(0, gpio_get_level(GPIO_NUM_4));
}

/*
 * ============================================================
 * GPIO6 - Sin carga conectada
 * ============================================================
 */

TEST_CASE("GPIO6 output without external load",
          "[gpio]")
{
    printf("\n");
    printf("========================================\n");
    printf(" GPIO6 - NO LOAD TEST\n");
    printf("========================================\n");

    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);

    /*
     * GPIO6 -> HIGH
     */
    printf("GPIO6 -> HIGH\n");

    GPIOOn(QUECTEL_PWRKEY_PIN);

    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("GPIO6 read: %d\n",
           gpio_get_level(GPIO_NUM_6));

    /*
     * GPIO6 -> LOW
     */
    printf("GPIO6 -> LOW\n");

    GPIOOff(QUECTEL_PWRKEY_PIN);

    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("GPIO6 read: %d\n",
           gpio_get_level(GPIO_NUM_6));

    /*
     * Al finalizar dejamos PWRKEY en LOW.
     */
    TEST_ASSERT_EQUAL(0, gpio_get_level(GPIO_NUM_6));
}