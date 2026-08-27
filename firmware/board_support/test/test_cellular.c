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

TEST_CASE("TEST-BSP-CELLULAR-07 Echo off", "[bsp][cellular][full]")
{
    UartHalInit(UART_BAUDRATE);


    bool full_on = CellularSetFullFunction();
    TEST_ASSERT_TRUE_MESSAGE( full_on, "El módulo no respondió OK al comando full");

    if (full_on) {
        printf("full activado correctamente.\n");
    }
}

TEST_CASE("TEST-BSP-CELLULAR-09 IMSI read", "[bsp][cellular][imsi]")
{
    UartHalInit(UART_BAUDRATE);

    char imsi[32];
    bool ok = CellularGetIMSI(imsi, sizeof(imsi));

    TEST_ASSERT_TRUE_MESSAGE(ok, "No se pudo obtener IMSI");

    if (ok) {
        printf("IMSI leído: %s\n", imsi);
    }
}


TEST_CASE("TEST-BSP-CELLULAR-10 Network registration", "[bsp][cellular][network]")
{
    UartHalInit(UART_BAUDRATE);

    printf("Waiting for cellular network registration...\n");

    bool registered = CellularWaitNetworkRegistration(120000); // 2 minutos

    TEST_ASSERT_TRUE_MESSAGE(registered, "ERROR: CellularWaitNetworkRegistration()");

    if (registered) {
        printf("Cellular network registered!\n");
    }
}


TEST_CASE("TEST-BSP-CELLULAR-11 Network registration status", "[bsp][cellular][cereg]")
{
    UartHalInit(UART_BAUDRATE);

    //TEST_ASSERT_TRUE_MESSAGE(CellularPowerOn(), "No se pudo encender el módulo");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000), "No se recibió RDY");

    int status;
    bool ok = CellularGetNetworkRegistration(&status);

    TEST_ASSERT_TRUE_MESSAGE(ok, "No se pudo obtener estado de registro");

    if (ok) {
        printf("Estado de registro de red: %d\n", status);
    }
}


TEST_CASE("TEST-BSP-CELLULAR-13 Signal quality", "[bsp][cellular][csq]")
{
    UartHalInit(UART_BAUDRATE);

    int rssi;
    bool ok = CellularGetSignalQuality(&rssi);

    TEST_ASSERT_TRUE_MESSAGE(ok,"No se pudo obtener calidad de señal");

    if (ok) {
        printf("RSSI leído: %d\n", rssi);
    }
}

TEST_CASE("TEST-BSP-CELLULAR-14 Operator name", "[bsp][cellular][cops]")
{
    UartHalInit(UART_BAUDRATE);

    //TEST_ASSERT_TRUE_MESSAGE(CellularPowerOn(),"No se pudo encender el módulo");

    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000),"No se recibió RDY");

    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitNetworkRegistration(120000),"No se registró en la red");

    char operator[64];
    bool ok = CellularGetOperator(operator, sizeof(operator));

    TEST_ASSERT_TRUE_MESSAGE( ok, "No se pudo obtener nombre de operador");

    if (ok) {
        printf("Operador actual: %s\n", operator);
    }
}


TEST_CASE("TEST-BSP-CELLULAR-15 Configure PDP context", "[bsp][cellular][pdp]")
{
    UartHalInit(UART_BAUDRATE);

    //TEST_ASSERT_TRUE_MESSAGE(CellularPowerOn(),"No se pudo encender el módulo");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000),"No se recibió RDY");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitNetworkRegistration(120000),"No se registró en la red");

    bool ok = CellularConfigurePdp(
        CELLULAR_APN,
        CELLULAR_USERNAME,
        CELLULAR_PASSWORD
    );

    TEST_ASSERT_TRUE_MESSAGE( ok, "No se pudo configurar PDP context");

    if (ok) {
        printf("PDP context configurado correctamente.\n");
    }
}


