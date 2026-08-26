#include "unity.h"

#include "cellular.h"
#include "uart_hal.h"
#include "board_config.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>


TEST_CASE(
    "TEST-BSP-CELLULAR-04 Power on module and wait RDY",
    "[bsp][cellular][poweron][ready]"
)
{
    printf("\n========================================\n");
    printf(" CELLULAR POWER ON + RDY TEST\n");
    printf("========================================\n");

    UartHalInit(UART_BAUDRATE);

    TEST_ASSERT_TRUE_MESSAGE(
        CellularPowerOn(),
        "No se pudo enviar pulso de encendido por PWRKEY"
    );

    printf("Esperando RDY...\n");

    bool ready = CellularWaitReady(10000);

    TEST_ASSERT_TRUE_MESSAGE(
        ready,
        "El modulo no envio RDY en el tiempo esperado"
    );

    if (ready) {
        printf("RDY recibido: el modulo esta listo.\n");
    }
}


TEST_CASE("TEST-BSP-CELLULAR-06 AT command ready", "[bsp][cellular][at]")
{
    UartHalInit(UART_BAUDRATE);

    //TEST_ASSERT_TRUE_MESSAGE( CellularPowerOn(), "No se pudo encender el módulo");

    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000), "No se recibió RDY");

    bool ready = CellularIsReady();
    TEST_ASSERT_TRUE_MESSAGE(ready, "El módulo no respondió OK al comando AT");

    if (ready) {
        printf("AT OK: módulo acepta comandos\n");
    }
}

TEST_CASE("TEST-BSP-CELLULAR-07 Echo off", "[bsp][cellular][echo]")
{
    UartHalInit(UART_BAUDRATE);

    //TEST_ASSERT_TRUE_MESSAGE(CellularPowerOn(),"No se pudo encender el módulo");

    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000),"No se recibió RDY");

    bool echo_off = CellularEchoOff();
    TEST_ASSERT_TRUE_MESSAGE( echo_off, "El módulo no respondió OK al comando ATE0");

    if (echo_off) {
        printf("Echo desactivado correctamente.\n");
    }
}