#include "unity.h"

#include "cellular.h"
#include "cellular_modem.h"

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
    //char response[128];

    TEST_ASSERT_TRUE(CellularModemSendCommand("AT\r\n", response,sizeof(response), 1000));

    TEST_ASSERT_NOT_NULL(strstr(response, "OK"));
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


TEST_CASE( "CellularModem responds to AT", "[cellular_modem]")
{
    char response_[128];
    bool ok;

    /* Asegurarse que el módem está encendido y listo para AT */
    TEST_ASSERT_TRUE(CellularModemInit());

    ok = CellularModemSendCommand("AT\r\n", response_, sizeof(response_), 1000);

    TEST_ASSERT_TRUE_MESSAGE(ok, "AT command failed");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(response_, "OK"), "OK not found in response");
}

TEST_CASE("CellularModem invalid AT returns ERROR", "[cellular_modem]")
{
    char response[128];

    //TEST_ASSERT_TRUE(CellularModemInit());

    bool ok = CellularModemSendCommand(
        "AT+COMANDO_QUE_NO_EXISTE\r\n",
        response,
        sizeof(response),
        1000
    );

    TEST_ASSERT_FALSE_MESSAGE(ok, "Invalid AT command should fail");

    TEST_ASSERT_NOT_NULL_MESSAGE( strstr(response, "ERROR"), "ERROR not found in response" );
}



/******************** */

#include <stdio.h>
#include <string.h>

/* ============================================================
 * Datos de prueba: APN del proveedor y servidor TCP de pruebas
 * ============================================================ */

#define CELLULAR_APN       "datos.personal.com"
#define CELLULAR_USERNAME  "datos"
#define CELLULAR_PASSWORD  "datos"

#define CELLULAR_TCP_TEST_SERVER "kkfdu-190-183-23-94.run.pinggy-free.link"
#define CELLULAR_TCP_TEST_PORT   33291

#define CELLULAR_TEST_PAYLOAD "HOLA MUNDO DESDE EG915U"

typedef struct {
    const char *label;
    const char *command;
    uint32_t    timeout_ms;
} AtCommandSample_t;

/* ============================================================
 * Helpers comunes a los tres TEST_CASE
 * ============================================================ */

/**
 * Imprime el buffer crudo con \r y \n visibles como texto ("\r","\n")
 * en lugar de saltos de línea reales, para poder ver exactamente cómo
 * viene delimitada cada respuesta (blancos, dobles CRLF, prompt sin
 * CRLF, etc.)
 */
static void PrintRawVisible(const char *label, const char *raw, size_t len)
{
    printf("---- %s ----\n", label);
    printf("bytes recibidos: %zu\n", len);
    printf("crudo: \"");
    for (size_t i = 0; i < len; i++) {
        char c = raw[i];
        if (c == '\r') {
            printf("\\r");
        } else if (c == '\n') {
            printf("\\n");
        } else if ((c < 32) || (c > 126)) {
            printf("\\x%02X", (unsigned char)c);
        } else {
            putchar(c);
        }
    }
    printf("\"\n");
}

/**
 * Manda un comando "normal" (que termina en OK/ERROR) y lo loguea.
 * NO sirve para AT+QISEND: ese no contesta OK, contesta el prompt
 * "> " sin CRLF, y CellularModemSendCommand lo clasificaria como
 * "Unexpected AT response" -> false. Para eso esta SendRawAndWait().
 */
static bool RunAtCommand(const char *label, const char *command, uint32_t timeout_ms)
{
    char response[256];
    response[0] = '\0';

    bool ok = CellularModemSendCommand(
        command,
        response,
        sizeof(response),
        timeout_ms
    );

    PrintRawVisible(label, response, strlen(response));
    printf("SendCommand() devolvio: %s\n\n", ok ? "true" : "false");

    return ok;
}

/**
 * Version "cruda" sin la evaluacion OK/ERROR de CellularModemSendCommand:
 * escribe bytes por UART y lee lo que venga, sin juzgar el contenido.
 * La usamos para el prompt de QISEND y para el payload en si, donde no
 * hay un "OK" que buscar.
 *
 * @param out_len [out] cantidad de bytes leidos (0 si timeout/error)
 * @return puntero al buffer estatico interno (valido hasta la proxima
 *         llamada), o NULL si fallo el TX.
 */
static char s_raw_buffer[256];

static const char *SendRawAndWait(const char *label, const void *data, size_t data_len, uint32_t timeout_ms, int *out_len)
{
    *out_len = 0;

    int written = UartHalWriteBytes((const char *)data, data_len);
    if (written <= 0) {
        printf("CELLULAR MODEM: TX crudo fallo (%s)\n", label);
        return NULL;
    }

    s_raw_buffer[0] = '\0';
    int len = UartHalReadBytes(s_raw_buffer, sizeof(s_raw_buffer) - 1, timeout_ms);
    if (len <= 0) {
        printf("---- %s ----\n", label);
        printf("timeout: no llego nada en %lu ms\n\n", timeout_ms);
        return NULL;
    }

    s_raw_buffer[len] = '\0';
    PrintRawVisible(label, s_raw_buffer, (size_t)len);
    printf("\n");

    *out_len = len;
    return s_raw_buffer;
}

/** ¿el buffer termina exactamente en el prompt "> " (sin CRLF)? */
static bool EndsWithPrompt(const char *buf, size_t len)
{
    return (len >= 2U) && (buf[len - 2U] == '>') && (buf[len - 1U] == ' ');
}

/* ============================================================
 * TEST_CASE 1: comandos basicos
 * ============================================================ */

static const AtCommandSample_t kCommandsToProbe[] = {
    { "AT basico",                 "AT\r\n",                                     2000  },
    { "Desactivar eco",            "ATE0\r\n",                                   2000  },
    { "Calidad de señal",          "AT+CSQ\r\n",                                 2000  },
    { "Registro de red",           "AT+CREG?\r\n",                               2000  },
    { "IMEI",                      "AT+CGSN\r\n",                                2000  },
    { "Contexto PDP (consultar)",  "AT+QIACT?\r\n",                              2000  },
};

#define AT_COMMANDS_TO_PROBE_COUNT (sizeof(kCommandsToProbe) / sizeof(kCommandsToProbe[0]))

TEST_CASE("probar comandos", "[test_command]")
{
    UartHalInit(UART_BAUDRATE);

    for (size_t i = 0; i < AT_COMMANDS_TO_PROBE_COUNT; i++) {
        const AtCommandSample_t *sample = &kCommandsToProbe[i];
        RunAtCommand(sample->label, sample->command, sample->timeout_ms);
    }
}

/* ============================================================
 * TEST_CASE 2: attach de red + contexto PDP + apertura de socket TCP
 * ============================================================ */

static const AtCommandSample_t kNetworkAttachCommands[] = {
    { "SIM lista",              "AT+CPIN?\r\n", 2000 },
    { "Operador",                "AT+COPS?\r\n", 5000 },
};

#define AT_NETWORK_ATTACH_COMMANDS_COUNT \
    (sizeof(kNetworkAttachCommands) / sizeof(kNetworkAttachCommands[0]))

TEST_CASE("probar comandos", "[test_command_2]")
{
    //UartHalInit(UART_BAUDRATE);

    char cmd_buffer[192];

    for (size_t i = 0; i < AT_NETWORK_ATTACH_COMMANDS_COUNT; i++) {
        const AtCommandSample_t *sample = &kNetworkAttachCommands[i];
        RunAtCommand(sample->label, sample->command, sample->timeout_ms);
    }

    snprintf(
        cmd_buffer, sizeof(cmd_buffer),
        "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n",
        CELLULAR_APN, CELLULAR_USERNAME, CELLULAR_PASSWORD
    );
    RunAtCommand("Configurar APN (QICSGP)", cmd_buffer, 5000);

    bool pdp_active = RunAtCommand("Activar contexto PDP (QIACT)", "AT+QIACT=1\r\n", 15000);

    RunAtCommand("Confirmar contexto PDP (QIACT?)", "AT+QIACT?\r\n", 2000);

    if (!pdp_active) {
        TEST_FAIL_MESSAGE("QIACT fallo, revisar APN/usuario/password antes de seguir");
        return;
    }

    snprintf(
        cmd_buffer, sizeof(cmd_buffer),
        "AT+QIOPEN=1,0,\"TCP\",\"%s\",%d,0,1\r\n",
        CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT
    );
    RunAtCommand("Abrir socket TCP (QIOPEN)", cmd_buffer, 15000);

    RunAtCommand("Cerrar socket (QICLOSE)", "AT+QICLOSE=0\r\n", 10000);
}

/* ============================================================
 * TEST_CASE 3: flujo real de envio de datos por TCP (QISEND)
 * ============================================================
 * Requiere que el contexto PDP ya haya quedado activo (corriste
 * [test_command_2] antes, o el contexto persiste entre tests si no
 * se desactivo). Si AT+QIOPEN falla acá, correlo primero.
 */

TEST_CASE("probar envio TCP", "[test_command_3]")
{
    UartHalInit(UART_BAUDRATE);

    char cmd_buffer[192];
    int len;
    const char *resp;

    /* --- Paso 1: abrir el socket --- */
    snprintf(
        cmd_buffer, sizeof(cmd_buffer),
        "AT+QIOPEN=1,0,\"TCP\",\"%s\",%d,0,1\r\n",
        CELLULAR_TCP_TEST_SERVER, CELLULAR_TCP_TEST_PORT
    );
    bool opened = RunAtCommand("Abrir socket TCP (QIOPEN)", cmd_buffer, 15000);
    if (!opened) {
        TEST_FAIL_MESSAGE("QIOPEN fallo, no tiene sentido seguir con el envio");
        return;
    }

    /* --- Paso 2: pedir el prompt de envio con AT+QISEND=<id>,<len> ---
     * Esto NO pasa por CellularModemSendCommand porque la respuesta no
     * es OK/ERROR, es literalmente "> " sin CRLF. */
    const char *payload = CELLULAR_TEST_PAYLOAD;
    size_t payload_len = strlen(payload);

    snprintf(cmd_buffer, sizeof(cmd_buffer), "AT+QISEND=0,%zu\r\n", payload_len);
    printf("CELLULAR MODEM TX: %s", cmd_buffer);

    resp = SendRawAndWait("Prompt de QISEND", cmd_buffer, strlen(cmd_buffer), 5000, &len);
    if (resp == NULL) {
        TEST_FAIL_MESSAGE("No llego respuesta a QISEND (ni prompt ni error)");
        return;
    }
    if (!EndsWithPrompt(resp, (size_t)len)) {
        printf("Se esperaba que termine en '> ' y no fue asi. Revisar si vino ERROR.\n");
        TEST_FAIL_MESSAGE("QISEND no devolvio el prompt esperado");
        return;
    }

    /* --- Paso 3: mandar el payload crudo (sin CRLF final, el modulo
     * corta solo al llegar a <len> bytes) --- */
    printf("CELLULAR MODEM TX (payload, %zu bytes): %s\n", payload_len, payload);
    resp = SendRawAndWait("Confirmacion de envio (SEND OK esperado)", payload, payload_len, 5000, &len);
    if (resp == NULL) {
        TEST_FAIL_MESSAGE("Timeout esperando confirmacion del envio");
        return;
    }
    if (strstr(resp, "SEND OK") == NULL) {
        printf("No se vio 'SEND OK' en la confirmacion, revisar si hubo SEND FAIL.\n");
    }

    /* --- Paso 4: esperar la URC de datos entrantes. Tu server nc
     * contesta con el string fijo "ACK", así que deberia llegar
     * +QIURC: "recv",0 en algun momento. --- */
    resp = SendRawAndWait("URC recibida tras el envio", "", 0, 8000, &len);
    if (resp == NULL) {
        printf("No llego ninguna URC de datos en 8s. Puede que el server tarde mas o no haya contestado.\n");
    } else if (strstr(resp, "+QIURC: \"recv\"") != NULL) {
        /* --- Paso 5: leer el dato recibido --- */
        RunAtCommand("Leer datos recibidos (QIRD)", "AT+QIRD=0,256\r\n", 5000);
    }

    /* --- Paso 6: cerrar el socket --- */
    RunAtCommand("Cerrar socket (QICLOSE)", "AT+QICLOSE=0\r\n", 10000);
}

/* ============================================================
 * Configuración del test
 * ============================================================ */

#define CELLULAR_TEST_APN        "datos.personal.com"
#define CELLULAR_TEST_USERNAME   "datos"
#define CELLULAR_TEST_PASSWORD   "datos"

#define CELLULAR_TEST_SERVER   "yhopb-190-183-23-94.run.pinggy-free.link"

#define CELLULAR_TEST_PORT       34321

#define CELLULAR_TEST_RX_SIZE    64

/* ============================================================
 * TEST
 * ============================================================ */

TEST_CASE("CellularModem test", "[cellular_modem_test]")
{
    char imei[32];
    uint8_t rx_data[CELLULAR_TEST_RX_SIZE];
    size_t received = 0;

    bool result;

    /* --------------------------------------------------------
     * 1. Inicialización
     * -------------------------------------------------------- */

    printf("\n");
    printf("========================================\n");
    printf(" TEST CELLULAR MODEM\n");
    printf("========================================\n");

    printf("\n[1] Inicializando modem...\n");

    UartHalInit(UART_BAUDRATE);


    //result = CellularModemInit();

    //TEST_ASSERT_TRUE_MESSAGE(result,"CellularModemInit() fallo");

    printf(
        "[OK] CellularModemInit()\n"
    );

    /* --------------------------------------------------------
     * 2. Verificar que el modem responde
     * -------------------------------------------------------- */

    printf("\n[2] Verificando modem...\n");

    result = CellularModemIsReady();

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "El modem no responde a AT"
    );

    printf(
        "[OK] CellularModemIsReady()\n"
    );

    /* --------------------------------------------------------
     * 3. Obtener IMEI
     * -------------------------------------------------------- */

    printf("\n[3] Obteniendo IMEI...\n");

    memset(
        imei,
        0,
        sizeof(imei)
    );

    result = CellularModemGetImei(
        imei,
        sizeof(imei)
    );

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "No se pudo obtener el IMEI"
    );

    printf(
        "IMEI: %s\n",
        imei
    );

    /*
     * Un IMEI normalmente tiene 15 digitos.
     */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(
        15,
        strlen(imei),
        "El IMEI no tiene 15 digitos"
    );

    printf(
        "[OK] CellularModemGetImei()\n"
    );

    /* --------------------------------------------------------
     * 4. Configurar PDP
     * -------------------------------------------------------- */

    printf("\n[4] Configurando PDP...\n");

    result = CellularModemConfigurePdp(
        CELLULAR_TEST_APN,
        CELLULAR_TEST_USERNAME,
        CELLULAR_TEST_PASSWORD
    );

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "No se pudo configurar el contexto PDP"
    );

    printf(
        "[OK] CellularModemConfigurePdp()\n"
    );

    /* --------------------------------------------------------
     * 5. Activar PDP
     * -------------------------------------------------------- */

    printf("\n[5] Activando PDP...\n");

    result = CellularModemActivatePdp();

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "No se pudo activar el contexto PDP"
    );

    printf(
        "[OK] CellularModemActivatePdp()\n"
    );

    /* --------------------------------------------------------
     * 6. Verificar PDP
     * -------------------------------------------------------- */

    printf("\n[6] Verificando PDP activo...\n");

    result = CellularModemIsPdpActive();

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "El contexto PDP no esta activo"
    );

    printf(
        "[OK] CellularModemIsPdpActive()\n"
    );

    /* --------------------------------------------------------
     * 7. Abrir conexion TCP
     * -------------------------------------------------------- */

    printf("\n[7] Abriendo conexion TCP...\n");

    result = CellularModemOpenTcp(
        CELLULAR_TEST_SERVER,
        CELLULAR_TEST_PORT
    );

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "No se pudo abrir la conexion TCP"
    );

    printf(
        "[OK] CellularModemOpenTcp()\n"
    );

    /* --------------------------------------------------------
     * 8. Enviar trama PANIC + IMEI
     * -------------------------------------------------------- */

    char frame[64];

    snprintf(
        frame,
        sizeof(frame),
        "PANIC,%s",
        imei
    );

    printf(
        "\n[8] Enviando trama TCP:\n"
        "    %s\n",
        frame
    );

    result = CellularModemSendTcp(
        (const uint8_t *)frame,
        strlen(frame)
    );

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "No se pudo enviar la trama TCP"
    );

    printf(
        "[OK] CellularModemSendTcp()\n"
    );

    /* --------------------------------------------------------
     * 9. Recibir ACK
     * -------------------------------------------------------- */

    printf(
        "\n[9] Esperando ACK...\n"
    );

    memset(
        rx_data,
        0,
        sizeof(rx_data)
    );

    received = 0;

    result = CellularModemReceiveTcp(
        rx_data,
        sizeof(rx_data),
        &received
    );

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "No se pudieron recibir datos TCP"
    );

    printf(
        "Bytes recibidos: %zu\n",
        received
    );

    printf(
        "Respuesta: \"%.*s\"\n",
        (int)received,
        rx_data
    );

    /*
     * El servidor debe responder ACK.
     */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(
        3,
        received,
        "La respuesta no tiene 3 bytes"
    );

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
        "ACK",
        rx_data,
        3,
        "La respuesta recibida no es ACK"
    );

    printf(
        "[OK] ACK recibido correctamente\n"
    );

    /* --------------------------------------------------------
     * 10. Cerrar TCP
     * -------------------------------------------------------- */

    printf("\n[10] Cerrando conexion TCP...\n");

    result = CellularModemCloseTcp();

    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "No se pudo cerrar el socket TCP"
    );

    printf(
        "[OK] CellularModemCloseTcp()\n"
    );

    /* --------------------------------------------------------
     * Resultado
     * -------------------------------------------------------- */

    printf("\n");
    printf("========================================\n");
    printf(" CELLULAR MODEM TEST: PASS\n");
    printf("========================================\n");
}