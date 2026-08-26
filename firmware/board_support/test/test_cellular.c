#include "unity.h"

#include "cellular.h"
#include "uart_hal.h"
#include "board_config.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>



TEST_CASE(
    "TEST-BSP-CELLULAR-01 Power on and AT response",
    "[bsp][cellular][power]"
)
{
    printf("\n");
    printf("========================================\n");
    printf(" CELLULAR POWER ON + AT TEST\n");
    printf("========================================\n");

    //UartHalInit(UART_BAUDRATE);


    /*
     * Inicializa UART y realiza la secuencia
     * de encendido mediante PWRKEY.
     */
    TEST_ASSERT_TRUE_MESSAGE(
        CellularInit(),
        "CellularInit() fallo"
   );

    /*
     * Dar tiempo al modulo para completar
     * el arranque.
     */
    vTaskDelay(
        pdMS_TO_TICKS(10000)
    );

    printf("Enviando AT...\n");
    

    

    const char *command = "AT\r\n";

    int written = UartHalWriteBytes(
        command,
        strlen(command)
    );

    printf(
        "Bytes enviados: %d\n",
        written
    );

    TEST_ASSERT_TRUE_MESSAGE(
        written > 0,
        "No se pudo transmitir AT"
    );

    /*
     * Esperamos la respuesta del módulo.
     */
    char response[256];

    int len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        5000
    );

    printf(
        "Bytes recibidos: %d\n",
        len
    );

    if (len > 0)
    {
        response[len] = '\0';

        printf("\n");
        printf("----------------------------------------\n");
        printf(" RESPUESTA DEL EG915U\n");
        printf("----------------------------------------\n");
        printf("%s\n", response);
        printf("----------------------------------------\n");
    }
    else
    {
        printf("\nNo se recibieron datos.\n");
    }

    /*
     * Por ahora NO comprobamos el contenido.
     *
     * Queremos observar primero exactamente
     * qué responde el módulo.
     */
    TEST_ASSERT_TRUE_MESSAGE(
        len > 0,
        "El EG915U no envio ninguna respuesta"
    );

    
}

TEST_CASE(
    "TEST-BSP-CELLULAR-02 Power off module",
    "[bsp][cellular][poweroff]"
)
{
    printf("\n");
    printf("========================================\n");
    printf(" CELLULAR POWER OFF TEST\n");
    printf("========================================\n");
    UartHalInit(UART_BAUDRATE);
    /* 
     * Nos aseguramos de que el módulo esté inicializado y encendido 
     * antes de intentar apagarlo.
     */
    //CellularInit();
    
    /* Le damos tiempo por si recién se encendió */
    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("Enviando comando de apagado (AT+QPOWD=1)...\n");

    /* 
     * AT+QPOWD=1 es el comando estándar de Quectel para un apagado 
     * normal (se desconecta de la red de forma segura antes de apagarse). 
     */
    const char *command = "AT+QPOWD=1\r\n";

    int written = UartHalWriteBytes(
        command,
        strlen(command)
    );

    printf(
        "Bytes enviados: %d\n",
        written
    );

    TEST_ASSERT_TRUE_MESSAGE(
        written > 0,
        "No se pudo transmitir el comando de apagado"
    );

    /*
     * Esperamos la respuesta del módulo.
     * Al apagar, suele responder "OK" y luego "NORMAL POWER DOWN".
     * Le damos un timeout generoso (ej. 8 segundos) porque desconectarse 
     * de la red celular puede tomar un par de segundos.
     */
    char response[256];

    int len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        8000 
    );

    printf(
        "Bytes recibidos: %d\n",
        len
    );

    if (len > 0)
    {
        response[len] = '\0';

        printf("\n");
        printf("----------------------------------------\n");
        printf(" RESPUESTA DEL EG915U AL APAGADO\n");
        printf("----------------------------------------\n");
        printf("%s\n", response);
        printf("----------------------------------------\n");
    }
    else
    {
        printf("\nNo se recibieron datos.\n");
    }

    /*
     * Comprobamos que el módulo haya devuelto algún mensaje confirmando
     * que inició su secuencia de apagado.
     */
    TEST_ASSERT_TRUE_MESSAGE(
        len > 0,
        "El EG915U no envio ninguna respuesta de apagado"
    );
}

TEST_CASE(
    "TEST-BSP-CELLULAR-01 Cellular initialization",
    "[bsp][cellular][init]"
)
{
    printf("\n");
    printf("========================================\n");
    printf(" CELLULAR INITIALIZATION TEST\n");
    printf("========================================\n");

    printf("Encendiendo EG915U...\n");
    printf("Esperando RDY...\n");

    TEST_ASSERT_TRUE_MESSAGE(
        CellularInit(),
        "El EG915U no envio RDY"
    );

    printf("EG915U listo.\n");
}


/////////////////////////////////////////////////

TEST_CASE(
    "TEST-BSP-CELLULAR-01 Power on and RDY",
    "[bsp][cellular][power_on]"
)
{
    printf("\n");
    printf("========================================\n");
    printf(" CELLULAR POWER ON + RDY TEST\n");
    printf("========================================\n");

    UartHalInit(UART_BAUDRATE);

    TEST_ASSERT_TRUE_MESSAGE(
        CellularPowerOn(),
        "CellularPowerOn() fallo"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        CellularWaitReady(12000),
        "No se recibio RDY del EG915U"
    );

    printf("EG915U encendido y listo para AT.\n");
}