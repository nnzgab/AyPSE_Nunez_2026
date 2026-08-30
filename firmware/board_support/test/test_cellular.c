#include "unity.h"

#include "cellular.h"
#include "uart_hal.h"
#include "board_config.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
/*
 void setUp(void)
{
    // Nada que inicializar antes de cada test por ahora.
}

void tearDown(void)
{
    CellularCloseAllSockets();
}
*/
/*
 * ============================================================================
 * CONVENCION DE ESTADO ENTRE TESTS
 * ============================================================================
 * Estos tests se corren de a UNO por tag, manualmente, vía unity_run_tests_by_tag
 * en test_app.c. No son independientes entre sí: cada uno asume que el módulo
 * quedó en el estado que dejó el test anterior DENTRO DE LA MISMA SESIÓN DE
 * ENERGÍA (mismo power-on, sin resets del MCU ni del módulo).
 *
 * Orden de estado acumulado:
 *   01 poweron         -> módulo encendido y RDY
 *   02 at              -> responde a comandos AT
 *   03 echo            -> echo desactivado
 *   04 full            -> función completa activada
 *   05 imsi            -> (no cambia estado, solo lectura)
 *   06 network         -> registrado en red
 *   07 cereg           -> (solo lectura)
 *   08 csq             -> (solo lectura)
 *   09 cops            -> (solo lectura, requiere red registrada)
 *   10 pdp             -> PDP configurado
 *   11 act-pdp         -> PDP activado
 *   12 pdp-status      -> (solo lectura, requiere PDP activo)
 *   13 tcp             -> abre y cierra un socket (requiere PDP activo)
 *   14 socket          -> diagnóstico de sockets vía túnel manual
 *   15 sendrecv        -> TCP send/recv usando AT crudo (bajo nivel)
 *   16 sendrecv_       -> ídem, esperando URC explícito (bajo nivel)
 *   17 send_           -> TCP send usando API del BSP (alto nivel)
 *   18 receivetcp_     -> TCP send+recv usando API del BSP (alto nivel)
 *   19 power-off       -> apaga el módulo (fin de sesión de trabajo)
 *
 * Si arrancás una sesión nueva (después de correr [power-off] o de un reset
 * del MCU/módulo), corré [poweron] antes de cualquier tag > 01. Las líneas
 * de setup que aparecen comentadas en los tests intermedios son justamente
 * eso: asumen que ya corriste el/los test(s) anteriores en esta sesión.
 *
 * IMPORTANTE: el apagado del módulo (GPIOOff) sólo debe ocurrir en el test
 * de [power-off]. Ningún otro test debe apagar el módulo por su cuenta.
 * ============================================================================
 */

/* ============================================================================
 * 01 - Encendido e inicialización
 * ========================================================================= */



TEST_CASE("TEST-BSP-CELLULAR-01 Power on module and wait RDY","[bsp][cellular][poweron][ready]")
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


/* ============================================================================
 * 02 - Comandos AT básicos
 * ========================================================================= */


TEST_CASE("TEST-BSP-CELLULAR-02 AT command ready", "[bsp][cellular][at]")
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

/* ============================================================================
 * 03 - Echo off
 * ========================================================================= */


TEST_CASE("TEST-BSP-CELLULAR-03 Echo off", "[bsp][cellular][echo]")
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

/* ============================================================================
 * 04 - Función completa
 * (antes duplicaba el nombre de 03 "Echo off" por error de copy-paste)
 * ========================================================================= */


TEST_CASE("TEST-BSP-CELLULAR-04 Set full function", "[bsp][cellular][full]")
{
    UartHalInit(UART_BAUDRATE);
    //TEST_ASSERT_TRUE_MESSAGE(CellularPowerOn(),"No se pudo encender el módulo");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000),"No se recibió RDY");

    bool full_on = CellularSetFullFunction();
    TEST_ASSERT_TRUE_MESSAGE( full_on, "El módulo no respondió OK al comando full");

    if (full_on) {
        printf("full activado correctamente.\n");
    }
}


/* ============================================================================
 * 05 - IMSI
 * ========================================================================= */

TEST_CASE("TEST-BSP-CELLULAR-05 IMSI read", "[bsp][cellular][imsi]")
{
    UartHalInit(UART_BAUDRATE);

    char imsi[32];
    bool ok = CellularGetIMSI(imsi, sizeof(imsi));

    TEST_ASSERT_TRUE_MESSAGE(ok, "No se pudo obtener IMSI");

    if (ok) {
        printf("IMSI leído: %s\n", imsi);
    }
}

/* ============================================================================
 * 06 - Registro de red
 * ========================================================================= */


TEST_CASE("TEST-BSP-CELLULAR-6 Network registration", "[bsp][cellular][network]")
{
    UartHalInit(UART_BAUDRATE);

    printf("Waiting for cellular network registration...\n");

    bool registered = CellularWaitNetworkRegistration(5000); // 2 minutos

    TEST_ASSERT_TRUE_MESSAGE(registered, "ERROR: CellularWaitNetworkRegistration()");

    if (registered) {
        printf("Cellular network registered!\n");
    }
}


/* ============================================================================
 * 07 - Estado de registro (CEREG)
 * ========================================================================= */


TEST_CASE("TEST-BSP-CELLULAR-7 Network registration status", "[bsp][cellular][cereg]")
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


/* ============================================================================
 * 08 - Calidad de señal (CSQ)
 * ========================================================================= */
 


TEST_CASE("TEST-BSP-CELLULAR-8 Signal quality", "[bsp][cellular][csq]")
{
    UartHalInit(UART_BAUDRATE);

    int rssi;
    bool ok = CellularGetSignalQuality(&rssi);

    TEST_ASSERT_TRUE_MESSAGE(ok,"No se pudo obtener calidad de señal");

    if (ok) {
        printf("RSSI leído: %d\n", rssi);
    }
}


/* ============================================================================
 * 09 - Nombre de operador (COPS)
 * ========================================================================= */

TEST_CASE("TEST-BSP-CELLULAR-9 Operator name", "[bsp][cellular][cops]")
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


/* ============================================================================
 * 10 - Configurar PDP context
 * ========================================================================= */

TEST_CASE("TEST-BSP-CELLULAR-10 Configure PDP context", "[bsp][cellular][pdp]")
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

/* ============================================================================
 * 11 - Activar PDP context
 * (se retiró el GPIOOff que apagaba el módulo acá — ver test 19 [power-off])
 * ========================================================================= */

TEST_CASE("TEST-BSP-CELLULAR-11 Activate PDP context", "[bsp][cellular][act-pdp]")
{
    UartHalInit(UART_BAUDRATE);

    //TEST_ASSERT_TRUE_MESSAGE( CellularPowerOn(), "No se pudo encender el módulo");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000),"No se recibió RDY");
    //TEST_ASSERT_TRUE_MESSAGE( CellularWaitNetworkRegistration(120000),"No se registró en la red");
    //TEST_ASSERT_TRUE_MESSAGE(CellularConfigurePdp(CELLULAR_APN, CELLULAR_USERNAME, CELLULAR_PASSWORD),"No se pudo configurar PDP context");

    bool ok = CellularActivatePdp();
    TEST_ASSERT_TRUE_MESSAGE( ok, "No se pudo activar PDP context");
    if (ok) {
        printf("PDP context activado correctamente.\n");
    }
}

/* ============================================================================
 * 12 - Estado del PDP context e IP asignada
 * ========================================================================= */

TEST_CASE("TEST-BSP-CELLULAR-12 PDP status and IP","[bsp][cellular][pdp-status]")
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
}

/* ============================================================================
 * 13 - Diagnóstico de apertura/cierre de socket TCP (sin envío de datos)
 * ========================================================================= */


TEST_CASE("TEST-BSP-CELLULAR-13 QIOPEN diagnostic", "[bsp][cellular][tcp]")
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
        CellularOpenSocket(socket_id, "TCP", CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),
        "No se pudo abrir la conexion TCP"
    );
    printf("\nTCP conectado correctamente.\n");

    TEST_ASSERT_TRUE_MESSAGE(
        CellularCloseSocket(socket_id),
        "No se pudo cerrar el socket TCP"
    );
    printf("TCP cerrado correctamente.\n");
}


/* ============================================================================
 * 14 - Diagnóstico de múltiples sockets vía túnel manual
 * NOTA MANUAL: requiere un túnel pinggy-free (u otro) activo y accesible.
 * Estos túneles gratuitos expiran cada pocas horas — si este test falla,
 * verificar primero si el túnel sigue vivo antes de sospechar del código.
 * ========================================================================= */


/*
TEST_CASE("TEST-BSP-CELLULAR-14 Socket open/close diagnostic (multi)", "[bsp][cellular][socket]")
{
    UartHalInit(UART_BAUDRATE);

    //TEST_ASSERT_TRUE_MESSAGE(CellularPowerOn(), "No se pudo encender el modulo");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitReady(10000), "No se recibio RDY");
    //TEST_ASSERT_TRUE_MESSAGE(CellularWaitNetworkRegistration(120000), "No se registro en la red");
    //TEST_ASSERT_TRUE_MESSAGE(CellularConfigurePdp(CELLULAR_APN, CELLULAR_USERNAME, CELLULAR_PASSWORD), "No se pudo configurar PDP");
    //TEST_ASSERT_TRUE_MESSAGE(CellularActivatePdp(), "No se pudo activar PDP");

    //Servidor de prueba manual,
    //ssh -p 443 -R0:localhost:8089 tcp@a.pinggy.io
    //nc -l -k -p 30000

   // pruebo el slot 0
    TEST_ASSERT_TRUE_MESSAGE(CellularOpenSocket(0, "TCP", CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT), "No se pudo abrir socket");

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
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m, "Todavía hay sockets abiertos despues de cerrar");
}
*/

/* ============================================================================
 * BAJO NIVEL (AT crudo) — pruebas de las primitivas QISEND / QIRD / URC
 * antes de confiar en ellas dentro de CellularSendTcp/CellularReceiveTcp.
 * ========================================================================= */
 
/* 15 - Send/receive por AT crudo (QISEND), lectura directa por UART */
/*
TEST_CASE("TEST-BSP-CELLULAR-15 TCP send/receive (raw AT)", "[bsp][cellular][tcp][sendrecv]")
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

            //CellularOpenTcp(socket_id, CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),

    TEST_ASSERT_TRUE_MESSAGE(
        CellularOpenSocket(socket_id, "TCP", CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),
        "No se pudo abrir la conexión TCP"
    );
    printf("\nTCP conectado correctamente.\n");

    // Enviar datos con QISEND
    char response[256];
    char command[64];

    snprintf(command, sizeof(command), "AT+QISEND=%d,%d\r\n", socket_id, (int)strlen(payload));
    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(command, response, sizeof(response), 800),
        "No se pudo iniciar QISEND"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(payload, response, sizeof(response), 800),
        "No se pudo enviar datos"
    );
    printf("TCP TX: %s\n", payload);

    // NOTA: contra un echo server real (ej. tcpbin.com) se espera "HELLO" de vuelta.
    // Contra el nc de prueba propio, ajustar el string esperado según lo que
    // ese servidor responda.


    // Recibir eco del servidor
    char recv_buf[256];
    int len = UartHalReadBytes(recv_buf, sizeof(recv_buf)-1, 10000);
    TEST_ASSERT_TRUE_MESSAGE(len > 0, "No se recibió respuesta del servidor");
    recv_buf[len] = '\0';
    printf("TCP RX: %s\n", recv_buf);

    TEST_ASSERT_TRUE_MESSAGE(strstr(recv_buf, "HELLO") != NULL, "El eco no contiene HELLO");

    // Cerrar socket
    TEST_ASSERT_TRUE_MESSAGE(
        CellularCloseSocket(socket_id),
        //CellularCloseTcp(socket_id),
        "No se pudo cerrar el socket TCP"
    );
    printf("TCP cerrado correctamente.\n");
}
*/


/* 16 - Send/receive por AT crudo, esperando el URC de recepción explícitamente */
/*
TEST_CASE("TEST-BSP-CELLULAR-16 TCP send/receive with URC wait (raw AT)", "[bsp][cellular][tcp][sendrecv_]")
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

    TEST_ASSERT_TRUE_MESSAGE(
        CellularOpenSocket(socket_id, "TCP", CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),
        "No se pudo abrir la conexión TCP"
    );
    printf("\nTCP conectado correctamente.\n");

    // Enviar datos
    char response[256];
    char command[64];

    snprintf(command, sizeof(command), "AT+QISEND=%d,%d\r\n", socket_id, (int)strlen(payload));
    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(command, response, sizeof(response), 800),
        "No se pudo iniciar QISEND"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendCommand(payload, response, sizeof(response), 800),
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
        CellularSendCommand(command, response, sizeof(response), 800),
        "No se pudo leer datos con QIRD"
    );

    printf("TCP RX: %s\n", response);

    // Contra el nc de prueba propio configurado para responder "chau"
    TEST_ASSERT_TRUE_MESSAGE(strstr(response, "chau") != NULL, "El eco no contiene 'chau'");

    // Cerrar socket
    TEST_ASSERT_TRUE_MESSAGE(
        CellularCloseSocket(socket_id),
        //CellularCloseTcp(socket_id),
        "No se pudo cerrar el socket TCP"
    );
    printf("TCP cerrado correctamente.\n");
}
*/

/* ============================================================================
 * ALTO NIVEL (API del BSP) — una vez validadas las primitivas AT arriba,
 * se prueba la API pública que las envuelve.
 * ========================================================================= */
 
/* 17 - Send únicamente, vía API de alto nivel */
/*
TEST_CASE("TEST-BSP-CELLULAR-17 TCP send (API)", "[bsp][cellular][tcp][send_]")
{
    UartHalInit(UART_BAUDRATE);

    bool active;
    char ip_address[64];

    TEST_ASSERT_TRUE_MESSAGE(
        CellularGetPdpStatus(
            &active,
            ip_address,
            sizeof(ip_address)
        ),
        "No se pudo consultar PDP"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        active,
        "El contexto PDP no está activo"
    );

    CellularCloseAllSockets();

    const int socket_id = 0;
    const char *payload = "HELLO\r\n";
        //CellularOpenTcp( socket_id, CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),

    TEST_ASSERT_TRUE_MESSAGE(

        CellularOpenSocket(socket_id, "TCP", CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),"No se pudo abrir la conexión TCP");

    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendTcp(
            socket_id,
            payload,
            strlen(payload)
        ),
        "No se pudieron enviar los datos TCP"
    );

    printf(
        "TCP TX: %s\n",
        payload
    );

    TEST_ASSERT_TRUE_MESSAGE(
        CellularCloseSocket(socket_id),
        //CellularCloseTcp(socket_id),
        "No se pudo cerrar el socket TCP"
    );
}
*/


/* 18 - Send + receive, vía API de alto nivel
 * (incluye todo lo que cubre el test 17, por eso ese no hace falta correrlo
 * aparte una vez que este pasa — se deja igual para poder aislar el envío
 * si algún día falla sólo la recepción)
 */


TEST_CASE(
    "TEST-BSP-CELLULAR-18 TCP send and receive (API)","[bsp][cellular][tcp][receivetcp_]")
{
    UartHalInit(UART_BAUDRATE);

    bool active;
    char ip_address[64];

    TEST_ASSERT_TRUE_MESSAGE(
        CellularGetPdpStatus(
            &active,
            ip_address,
            sizeof(ip_address)
        ),
        "No se pudo consultar PDP"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        active,
        "El contexto PDP no está activo"
    );

    printf(
        "\nPDP activo. IP: %s\n",
        ip_address
    );

    CellularCloseAllSockets();

    const int socket_id = 0;
    const char *payload = "HELLO\r\n";


    TEST_ASSERT_TRUE_MESSAGE(
        CellularOpenSocket(socket_id, "TCP", CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT),
        "No se pudo abrir la conexión TCP"
    );

    printf(
        "\nTCP conectado correctamente.\n"
    );


    TEST_ASSERT_TRUE_MESSAGE(
        CellularSendTcp(
            socket_id,
            payload,
            strlen(payload)
        ),
        "No se pudieron enviar los datos TCP"
    );

    printf(
        "TCP TX: %s\n",
        payload
    );

 
    char response[256];

    TEST_ASSERT_TRUE_MESSAGE(
        CellularReceiveTcp(
            socket_id,
            response,
            sizeof(response)
        ),
        "No se pudieron recibir los datos TCP"
    );

    printf(
        "TCP RX: %s\n",
        response
    );


    TEST_ASSERT_TRUE_MESSAGE(
        strstr(response, "chau") != NULL,
        "La respuesta TCP no contiene 'chau'"
    );

 
    TEST_ASSERT_TRUE_MESSAGE(
        CellularCloseSocket(socket_id),
        //CellularCloseTcp(socket_id),
        "No se pudo cerrar el socket TCP"
    );

    printf(
        "TCP cerrado correctamente.\n"
    );
}


/* ============================================================================
 * 19 - Apagado del módulo (fin de sesión de trabajo)
 * Este es el ÚNICO lugar donde el módulo debe apagarse.
 * ========================================================================= */
 
TEST_CASE("TEST-BSP-CELLULAR-19 Power off module", "[bsp][cellular][power-off]")
{
    UartHalInit(UART_BAUDRATE);

    printf("\n========== POWER OFF TEST ==========\n");
    printf("Apagando el modulo celular...\n");

    bool ok = CellularPowerOff();

    TEST_ASSERT_TRUE_MESSAGE(ok, "No se pudo apagar el modulo (ni por software ni por hardware)");

    printf("\nModulo apagado.\n");
    printf(
        "NOTA: sin pin STATUS conectado, no se puede confirmar "
        "el apagado por software. Verificar manualmente si el "
        "modulo respondio con OK+POWERED DOWN o si se uso el "
        "failsafe por hardware, revisando el log de "
        "CellularPowerOff.\n"
    );
}

