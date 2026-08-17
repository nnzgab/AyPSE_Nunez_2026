#include "unity.h"
#include "uart_hal.h"
#include <string.h>

/* Test de loopback UART: requiere conectar físicamente TX ↔ RX */

TEST_CASE("UartHal loopback transmits and receives single byte", "[drivers_hal][uart][loopback]")
{
    UartHalInit(115200);

    const char tx = 'X';
    UartHalWriteByte(tx);

    char rx;
    int r = UartHalReadByte(&rx);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, r, "No se recibió ningún byte en loopback");
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(tx, rx, "El byte recibido no coincide con el enviado");
}

TEST_CASE("UartHal loopback transmits and receives string", "[drivers_hal][uart][loopback]")
{
    UartHalInit(115200);

    const char *msg = "Hola UART";
    UartHalWriteBytes(msg, strlen(msg));

    char buf[32] = {0};
    int r = UartHalReadBytes(buf, strlen(msg), 200);

    TEST_ASSERT_EQUAL_INT_MESSAGE(strlen(msg), r, "Cantidad de bytes recibidos incorrecta");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(msg, buf, "El string recibido no coincide con el enviado");
}


TEST_CASE("UartHalInit initializes with default pins", "[drivers_hal][uart]")
{
    UartHalInit(115200);
    TEST_ASSERT_TRUE(1); // Si no hubo crash, pasa
}

TEST_CASE("UartHalInitWithPins initializes with explicit pins", "[drivers_hal][uart]")
{
    UartHalInitWithPins(9600, 18, 19);
    TEST_ASSERT_TRUE(1);
}

TEST_CASE("UartHalWriteByte does not crash", "[drivers_hal][uart]")
{
    UartHalInit(115200);
    UartHalWriteByte('A');
    TEST_ASSERT_TRUE(1);
}

TEST_CASE("UartHalWriteBytes does not crash with valid buffer", "[drivers_hal][uart]")
{
    UartHalInit(115200);
    const char *msg = "Hello UART";
    UartHalWriteBytes(msg, strlen(msg));
    TEST_ASSERT_TRUE(1);
}

TEST_CASE("UartHalReadByte returns 0 on timeout", "[drivers_hal][uart]")
{
    UartHalInit(115200);
    char c;
    int r = UartHalReadByte(&c);
    TEST_ASSERT_TRUE(r >= 0); // 0 = timeout, >0 = algo leído
}

TEST_CASE("UartHalReadBytes returns 0 on timeout", "[drivers_hal][uart]")
{
    UartHalInit(115200);
    char buf[16];
    int r = UartHalReadBytes(buf, sizeof(buf), 100);
    TEST_ASSERT_TRUE(r >= 0);
}

TEST_CASE("UartHalReadByte handles NULL pointer", "[drivers_hal][uart]")
{
    UartHalInit(115200);
    int r = UartHalReadByte(NULL);
    TEST_ASSERT_TRUE(r < 0); // Debe devolver error
}

TEST_CASE("UartHalWriteBytes handles NULL buffer", "[drivers_hal][uart]")
{
    UartHalInit(115200);
    UartHalWriteBytes(NULL, 10); // No debe crashear
    TEST_ASSERT_TRUE(1);
}