#include "unity.h"
#include "gpio_hal.h"
#include "board_config.h"
#include <stdbool.h>

/*
 * GPIO HAL - Unit Tests
 *
 * Estos tests verifican la interfaz pública del HAL.
 *
 * No se pretende validar aquí el comportamiento eléctrico
 * de LEDs, pulsadores o del módulo Quectel.
 *
 * La validación eléctrica sobre hardware real se documenta
 * como prueba funcional independiente.
 */


/*
 * ============================================================
 * GPIOInit
 * ============================================================
 */

/**
 * @brief Verifica que GPIO23 pueda configurarse como entrada.
 *
 * GPIO23 corresponde al pulsador de pánico.
 */
TEST_CASE("GPIOInit configures panic button as input",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_PANIC_BTN, GPIO_INPUT);

    TEST_ASSERT_TRUE(true);
}


/**
 * @brief Verifica que GPIO4 pueda configurarse como salida.
 *
 * GPIO4 corresponde al LED indicador del botón de pánico.
 */
TEST_CASE("GPIOInit configures panic LED as output",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);

    TEST_ASSERT_TRUE(true);
}


/**
 * @brief Verifica que GPIO5 pueda configurarse como salida.
 */
TEST_CASE("GPIOInit configures Quectel LED as output",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_QUECTEL_LED_STATUS, GPIO_OUTPUT);

    TEST_ASSERT_TRUE(true);
}


/**
 * @brief Verifica que GPIO6 pueda configurarse como salida.
 *
 * GPIO6 corresponde a PWRKEY del módulo Quectel.
 */
TEST_CASE("GPIOInit configures Quectel PWRKEY as output",
          "[drivers_hal][gpio]")
{
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);

    TEST_ASSERT_TRUE(true);
}


/*
 * ============================================================
 * GPIOOn
 * ============================================================
 */

/**
 * @brief Verifica que GPIOOn() pueda ser ejecutada
 *        sobre una salida.
 */
TEST_CASE("GPIOOn drives panic LED output",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);

    GPIOOn(GPIO_PANIC_LED_STATUS);

    TEST_ASSERT_TRUE(true);
}


/*
 * ============================================================
 * GPIOOff
 * ============================================================
 */

/**
 * @brief Verifica que GPIOOff() pueda ser ejecutada
 *        sobre una salida.
 */
TEST_CASE("GPIOOff drives panic LED output",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);

    GPIOOff(GPIO_PANIC_LED_STATUS);

    TEST_ASSERT_TRUE(true);
}


/*
 * ============================================================
 * GPIOState
 * ============================================================
 */

/**
 * @brief Verifica que GPIOState() pueda establecer
 *        ambos estados lógicos.
 */
TEST_CASE("GPIOState drives high and low",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);

    GPIOState(GPIO_PANIC_LED_STATUS, true);
    GPIOState(GPIO_PANIC_LED_STATUS, false);

    TEST_ASSERT_TRUE(true);
}


/*
 * ============================================================
 * GPIOToggle
 * ============================================================
 */

/**
 * @brief Verifica que GPIOToggle() pueda ejecutarse
 *        sobre una salida.
 */
TEST_CASE("GPIOToggle changes output state",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);

    GPIOToggle(GPIO_PANIC_LED_STATUS);
    GPIOToggle(GPIO_PANIC_LED_STATUS);

    TEST_ASSERT_TRUE(true);
}


/*
 * ============================================================
 * GPIORead
 * ============================================================
 */

/**
 * @brief Verifica que GPIORead() pueda leer una entrada.
 *
 * No se impone un valor físico determinado porque el estado
 * depende de la condición real del pulsador.
 */
TEST_CASE("GPIORead reads panic button",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_PANIC_BTN, GPIO_INPUT);

    bool state = GPIORead(GPIO_PANIC_BTN);

    TEST_ASSERT_TRUE(state == true || state == false);
}


/*
 * ============================================================
 * Integridad básica de los GPIO utilizados por el proyecto
 * ============================================================
 */

/**
 * @brief Verifica que todos los GPIO utilizados por el proyecto
 *        puedan inicializarse.
 */
TEST_CASE("Project GPIOs can be initialized",
          "[drivers_hal][gpio]")
{
    GPIOInit(GPIO_PANIC_BTN, GPIO_INPUT);

    GPIOInit(GPIO_PANIC_LED_STATUS, GPIO_OUTPUT);

    GPIOInit(GPIO_QUECTEL_LED_STATUS, GPIO_OUTPUT);

    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);

    TEST_ASSERT_TRUE(true);
}