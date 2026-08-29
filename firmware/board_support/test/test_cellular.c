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

    TEST_ASSERT_TRUE_MESSAGE(CellularPowerOn(),"No se pudo encender el módulo");
    TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000),"No se recibió RDY");
    TEST_ASSERT_TRUE_MESSAGE(CellularWaitNetworkRegistration(120000),"No se registró en la red");

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


TEST_CASE("TEST-BSP-CELLULAR-16 Activate PDP context", "[bsp][cellular][act-pdp]")
{
    UartHalInit(UART_BAUDRATE);

    TEST_ASSERT_TRUE_MESSAGE( CellularPowerOn(), "No se pudo encender el módulo");

    //printf("hola///////////gdfg///////.\n");
    //GPIOOff(QUECTEL_PWRKEY_PIN);

    TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000),"No se recibió RDY");
    TEST_ASSERT_TRUE_MESSAGE( CellularWaitNetworkRegistration(120000),"No se registró en la red");
    TEST_ASSERT_TRUE_MESSAGE(CellularConfigurePdp(CELLULAR_APN, CELLULAR_USERNAME, CELLULAR_PASSWORD),"No se pudo configurar PDP context");

    bool ok = CellularActivatePdp();
    GPIOOff(QUECTEL_PWRKEY_PIN);

    TEST_ASSERT_TRUE_MESSAGE( ok, "No se pudo activar PDP context");

    if (ok) {
        printf("PDP context activado correctamente.\n");
    }
  
}


TEST_CASE(
    "TEST-BSP-CELLULAR-17 PDP status and IP",
    "[bsp][cellular][pdp-status]"
)
{
    UartHalInit(UART_BAUDRATE);

//    TEST_ASSERT_TRUE_MESSAGE( CellularPowerOn(), "No se pudo encender el módulo");

//    TEST_ASSERT_TRUE_MESSAGE( CellularWaitReady(10000), "No se recibió RDY");

//    TEST_ASSERT_TRUE_MESSAGE( CellularWaitNetworkRegistration(120000), "No se registró en la red");
/*
    TEST_ASSERT_TRUE_MESSAGE(
        CellularConfigurePdp(
            CELLULAR_APN,
            CELLULAR_USERNAME,
            CELLULAR_PASSWORD
        ),
        "No se pudo configurar PDP"
    );
*/
//    TEST_ASSERT_TRUE_MESSAGE( CellularActivatePdp(), "No se pudo activar PDP");

    bool active;
    char ip_address[64];

    TEST_ASSERT_TRUE_MESSAGE(
        CellularGetPdpStatus(
            &active,
            ip_address,
            sizeof(ip_address)
        ),
        "No se pudo consultar estado PDP"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        active,
        "El contexto PDP no está activo"
    );

    printf(
        "PDP activo. IP asignada: %s\n",
        ip_address
    );

    //GPIOOff(QUECTEL_PWRKEY_PIN);
}



TEST_CASE(
    "TEST-BSP-CELLULAR-18 QIOPEN diagnostic",
    "[bsp][cellular][tcp]"
)
{
    UartHalInit(UART_BAUDRATE);

    bool active;
    char ip_address[64];

    TEST_ASSERT_TRUE_MESSAGE(
        CellularGetPdpStatus(&active, ip_address, sizeof(ip_address)),
        "No se pudo consultar PDP"
    );
    TEST_ASSERT_TRUE_MESSAGE(active, "El contexto PDP no esta activo");
    printf("\nPDP activo. IP: %s\n", ip_address);

    /*
     * Limpieza defensiva: siempre, no comentada.
     * Cubre restos de tests anteriores, resets del MCU, etc.
     */
    CellularPrintSocketState();
    CellularCloseAllSockets();
    CellularPrintSocketState();

    const int socket_id = 0;

    TEST_ASSERT_TRUE_MESSAGE(
        CellularOpenTcp(socket_id, CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),
        "No se pudo abrir la conexion TCP"
    );
    printf("\nTCP conectado correctamente.\n");

    TEST_ASSERT_TRUE_MESSAGE(
        CellularCloseTcp(socket_id),
        "No se pudo cerrar el socket TCP"
    );
    printf("TCP cerrado correctamente.\n");
}




TEST_CASE(
    "TEST-BSP-CELLULAR-19 Power off module",
    "[bsp][cellular][power-off]"
)
{
    UartHalInit(UART_BAUDRATE);

    printf("\n========== POWER OFF TEST ==========\n");
    printf("Apagando el modulo celular...\n");

    bool ok = CellularPowerOff();

    TEST_ASSERT_TRUE_MESSAGE(
        ok,
        "No se pudo apagar el modulo (ni por software ni por hardware)"
    );

    printf("\nModulo apagado.\n");
    printf(
        "NOTA: sin pin STATUS conectado, no se puede confirmar "
        "el apagado por software. Verificar manualmente si el "
        "modulo respondio con OK+POWERED DOWN o si se uso el "
        "failsafe por hardware, revisando el log de "
        "CellularPowerOff.\n"
    );
}

TEST_CASE("TEST-BSP-CELLULAR-20 Socket open/close diagnostic", "[bsp][cellular][socket]")
{
    UartHalInit(UART_BAUDRATE);

    //TEST_ASSERT_TRUE_MESSAGE(CellularPowerOn(), "No se pudo encender el modulo");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000), "No se recibio RDY");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitNetworkRegistration(120000), "No se registro en la red");
    //TEST_ASSERT_TRUE_MESSAGE(CellularConfigurePdp(CELLULAR_APN, CELLULAR_USERNAME, CELLULAR_PASSWORD), "No se pudo configurar PDP");
    //TEST_ASSERT_TRUE_MESSAGE(CellularActivatePdp(), "No se pudo activar PDP");

    // Abrimos un socket TCP de prueba (ejemplo: google.com:80)
    //ssh -p 443 -R0:localhost:8089 tcp@a.pinggy.io
    //nc -l -k -p 30000

    //│  tcp://qextg-190-183-23-94.run.pinggy-free.link:34553           

    TEST_ASSERT_TRUE_MESSAGE(CellularOpenSocket(0, "TCP", "tcp://qextg-190-183-23-94.run.pinggy-free.link", 34553), "No se pudo abrir socket");

    // Consultamos sockets abiertos
    int sockets[10];
    int n = CellularGetOpenSockets(sockets, 10);

    TEST_ASSERT_TRUE_MESSAGE(n > 0, "No se detectaron sockets abiertos");

    bool found = false;
    for (int i = 0; i < n; i++) {
        if (sockets[i] == 0) {
            found = true;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(found, "Socket 0 no aparece en lista de abiertos");

    // Cerramos todos los sockets detectados
    for (int i = 0; i < n; i++) {
        TEST_ASSERT_TRUE_MESSAGE(CellularCloseSocket(sockets[i]), "No se pudo cerrar socket");
    }

    // Verificamos que ya no haya sockets abiertos
    int m = CellularGetOpenSockets(sockets, 10);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m, "Todavía hay sockets abiertos después de cerrar");
}


TEST_CASE(
    "TEST-BSP-CELLULAR-21 TCP send/receive diagnostic",
    "[bsp][cellular][tcp][sendrecv]"
)
{
    UartHalInit(UART_BAUDRATE);

    bool active;
    char ip_address[64];

    TEST_ASSERT_TRUE_MESSAGE(
        CellularGetPdpStatus(&active, ip_address, sizeof(ip_address)),
        "No se pudo consultar PDP"
    );
    TEST_ASSERT_TRUE_MESSAGE(active, "El contexto PDP no está activo");
    printf("\nPDP activo. IP: %s\n", ip_address);

    // Limpieza defensiva
    CellularCloseAllSockets();

    const int socket_id = 0;
    const char *payload = "HELLO\r\n";

    // Abrir conexión TCP
    TEST_ASSERT_TRUE_MESSAGE(
        CellularOpenTcp(socket_id, CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),
        "No se pudo abrir la conexión TCP"
    );
    printf("\nTCP conectado correctamente.\n");

    // Enviar datos con QISEND
    char response[256];
    char command[64];

    snprintf(command, sizeof(command), "AT+QISEND=%d,%d\r\n", socket_id, (int)strlen(payload));
    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(command, response, sizeof(response), 5000),
        "No se pudo iniciar QISEND"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(payload, response, sizeof(response), 5000),
        "No se pudo enviar datos"
    );
    printf("TCP TX: %s\n", payload);

    // Recibir eco del servidor
    char recv_buf[256];
    int len = UartHalReadBytes(recv_buf, sizeof(recv_buf)-1, 10000);
    TEST_ASSERT_TRUE_MESSAGE(len > 0, "No se recibió respuesta del servidor");
    recv_buf[len] = '\0';
    printf("TCP RX: %s\n", recv_buf);

    TEST_ASSERT_TRUE_MESSAGE(strstr(recv_buf, "HELLO") != NULL, "El eco no contiene HELLO");

    // Cerrar socket
    TEST_ASSERT_TRUE_MESSAGE(
        CellularCloseTcp(socket_id),
        "No se pudo cerrar el socket TCP"
    );
    printf("TCP cerrado correctamente.\n");
}


TEST_CASE(
    "TEST-BSP-CELLULAR-23 TCP send/receive with URC wait",
    "[bsp][cellular][tcp][sendrecv_]"
)
{
    UartHalInit(UART_BAUDRATE);

    bool active;
    char ip_address[64];

    TEST_ASSERT_TRUE_MESSAGE(
        CellularGetPdpStatus(&active, ip_address, sizeof(ip_address)),
        "No se pudo consultar PDP"
    );
    TEST_ASSERT_TRUE_MESSAGE(active, "El contexto PDP no está activo");
    printf("\nPDP activo. IP: %s\n", ip_address);

    CellularCloseAllSockets();

    const int socket_id = 0;
    const char *payload = "HELLO\r\n";

    // Abrir conexión TCP
    TEST_ASSERT_TRUE_MESSAGE(
        CellularOpenTcp(socket_id, CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),
        "No se pudo abrir la conexión TCP"
    );
    printf("\nTCP conectado correctamente.\n");

    // Enviar datos
    char response[256];
    char command[64];

    snprintf(command, sizeof(command), "AT+QISEND=%d,%d\r\n", socket_id, (int)strlen(payload));
    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(command, response, sizeof(response), 5000),
        "No se pudo iniciar QISEND"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(payload, response, sizeof(response), 5000),
        "No se pudo enviar datos"
    );
    printf("TCP TX: %s\n", payload);

    // Esperar explícitamente el URC de recepción
    TEST_ASSERT_TRUE_MESSAGE(
        CellularWaitForResponse("+QIURC: \"recv\",0", response, sizeof(response), 15000),
        "No llegó el URC de recepción"
    );

    // Leer datos recibidos con QIRD
    snprintf(command, sizeof(command), "AT+QIRD=%d,1500\r\n", socket_id);
    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(command, response, sizeof(response), 10000),
        "No se pudo leer datos con QIRD"
    );

    printf("TCP RX: %s\n", response);
    TEST_ASSERT_TRUE_MESSAGE(strstr(response, "chau") != NULL, "El eco no contiene 'chau'");

    // Cerrar socket
    TEST_ASSERT_TRUE_MESSAGE(
        CellularCloseTcp(socket_id),
        "No se pudo cerrar el socket TCP"
    );
    printf("TCP cerrado correctamente.\n");
}
