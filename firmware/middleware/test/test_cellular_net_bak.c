/*
#include "unity.h"
#include "cellular_net.h"
#include "cellular_modem.h"
#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
*/
/*

void setUp(void)
{
    // 1. Capa BSP: Inicializamos el hardware físico primero (igual que en los tests de BSP)
    CellularModemInit();
}

void tearDown(void)
{
    // Limpieza si es necesaria entre tests
}


TEST_CASE("TEST-MW-NET-01 CellularNet Initialization", "[middleware][cellular]") {
    cellular_net_err_t err = CellularNet_Init();
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_OK, err, "Fallo la inicializacion de cellular_net");

    cellular_net_status_t status = CellularNet_GetStatus();
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELL_STATE_OFF, status.current_state, "El estado inicial deberia ser CELL_STATE_OFF");
    TEST_ASSERT_FALSE_MESSAGE(CellularNet_IsReady(), "El sistema no debe reportar estar listo en el arranque");
}

TEST_CASE("TEST-MW-NET-02 Alert Frame Parameter Validation", "[middleware][cellular]") {
    // Aseguramos que este inicializado
    CellularNet_Init();

    uint8_t dummy_payload[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Caso A: Enviar con el módem apagado (debe rechazarlo por estado)
    cellular_net_err_t err = CellularNet_SendAlertFrame(dummy_payload, sizeof(dummy_payload));
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_ERR_NOT_CONNECTED, err, "Deberia rechazar el envio si no esta conectado");

    // Caso B: Puntero nulo o longitud cero
    TEST_ASSERT_EQUAL(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(NULL, sizeof(dummy_payload)));
    TEST_ASSERT_EQUAL(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(dummy_payload, 0));
    
    // Caso C: Exceder el tamaño máximo permitido (MAX_PAYLOAD_SIZE = 256)
    uint8_t large_payload[300] = {0};
    TEST_ASSERT_EQUAL(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(large_payload, sizeof(large_payload)));
}

TEST_CASE("TEST-MW-NET-03 Alert Queue Integrity", "[middleware][cellular]") {
    CellularNet_Init();
    
    // Forzamos temporalmente el estado interno a conectado para probar la inserción en cola
    // (Nota: Si el struct es estático en cellular_net.c, idealmente exponemos un setter de prueba 
    // o testeamos mediante la ejecución de la tarea. Aquí validamos el comportamiento de rechazo de parámetros).
    
    uint8_t payload[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    
    // Con estado OFF, debe fallar con ERR_NOT_CONNECTED
    TEST_ASSERT_EQUAL(CELL_NET_ERR_NOT_CONNECTED, CellularNet_SendAlertFrame(payload, sizeof(payload)));
}

TEST_CASE("TEST-MW-NET-04 Full State Machine Boot & AT Flow", "[middleware][cellular][interactive][hardware]") {
    printf("\n[ATENCION] Asegurese de que el modem este conectado fisicamente.\n");
    printf("Iniciando la capa de red y lanzando la tarea de estado...\n");

    // 1. Inicializamos la red y las colas
    TEST_ASSERT_EQUAL(CELL_NET_OK, CellularNet_Init());

    // 2. Creamos temporalmente la tarea real del middleware para que empiece a gobernar el estado
    TaskHandle_t net_task_handle = NULL;
    BaseType_t ret = xTaskCreate(CellularNet_Task, "CellNetTask", 4096, NULL, 5, &net_task_handle);
    TEST_ASSERT_EQUAL_MESSAGE(pdPASS, ret, "Fallo al crear la tarea CellularNet_Task");

    printf("--> Tarea de red corriendo. Monitoreando transiciones de estado por 15 segundos...\n");

    // 3. Monitoreamos dinámicamente el progreso de la máquina de estados
    cellular_net_status_t current_status;
    cellular_net_state_t last_logged_state = (cellular_net_state_t)-1;
    
    for (int i = 0; i < 30; i++) { // 30 iteraciones de 500ms = 15 segundos
        current_status = CellularNet_GetStatus();
        
        if (current_status.current_state != last_logged_state) {
            printf("[ESTADO CAMBIO] Nuevo estado detectado en la maquina: %d (Errores consec.: %d)\n", 
                   current_status.current_state, current_status.consecutive_errors);
            last_logged_state = current_status.current_state;
        }

        // Si llega a READY/CONNECTED, el arranque AT fue exitoso
        if (CellularNet_IsReady()) {
            printf("--> ¡Exito! La maquina de estados llego al estado CELL_STATE_CONNECTED_IDLE.\n");
            vTaskDelete(net_task_handle); // Limpiamos la tarea de prueba
            TEST_PASS();
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Si pasaron los 15 segundos y no conectó, levantamos fallo mostrando en qué estado quedó
    current_status = CellularNet_GetStatus();
    printf("--> [FALLO] Timeout esperando conexion. El estado final atascado fue: %d\n", current_status.current_state);
    
    if (net_task_handle != NULL) {
        vTaskDelete(net_task_handle);
    }
    
    TEST_FAIL_MESSAGE("La maquina de estados no logro completar el flujo de inicializacion y registro.");
}

TEST_CASE("TEST-MW-NET-INIT-DEMO Init & Base State Inspection", "[middleware1][cellular][interactive]") {
    printf("\n========================================\n");
    printf("[TEST INIT] Iniciando prueba de inicializacion y estado base...\n");
    printf("========================================\n");

    // 1. Inicializar la capa de red (crea la cola y setea variables base)
    cellular_net_err_t err = CellularNet_Init();
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_OK, err, "Fallo critico al ejecutar CellularNet_Init");
    printf("[TEST INIT] -> CellularNet_Init() ejecutado correctamente (OK).\n");

    // 2. Consultar y mostrar el estado actual en la consola
    cellular_net_status_t status = CellularNet_GetStatus();
    printf("[TEST INIT] -> Estado actual reportado: %d (Valor esperado: 0 para CELL_STATE_OFF)\n", status.current_state);
    
    // Validar que nazca en OFF
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELL_STATE_OFF, status.current_state, "El estado inicial no es CELL_STATE_OFF");

    // 3. Consultar la bandera de disponibilidad
    bool is_ready = CellularNet_IsReady();
    printf("[TEST INIT] -> CellularNet_IsReady() devuelve: %s (Valor esperado: false)\n", is_ready ? "VERDADERO" : "FALSO");
    
    // Validar que no esté listo
    TEST_ASSERT_FALSE_MESSAGE(is_ready, "El sistema reporta estar listo prematuramente en el arranque");

    printf("========================================\n");
    printf("[TEST INIT] ¡Prueba de inicializacion superada con exito!\n");
    printf("========================================\n");
    
    TEST_PASS();
}

TEST_CASE("TEST-MW-NET-PARAM-DEMO Alert Frame Parameter & State Validation", "[middleware2][cellular][interactive]") {
    printf("\n========================================\n");
    printf("[TEST PARAM] Iniciando validacion de parametros y fronteras...\n");
    printf("========================================\n");

    // Asegurar estado limpio inicial
    CellularNet_Init();

    uint8_t dummy_payload[16] = "AlarmaPaTest";

    // 1. Probar rechazo por estado desconectado (El estado actual es OFF)
    cellular_net_err_t err = CellularNet_SendAlertFrame(dummy_payload, sizeof(dummy_payload));
    printf("[TEST PARAM] -> Envio con estado OFF devuelve: %d (Esperado: %d para ERR_NOT_CONNECTED)\n", 
           err, CELL_NET_ERR_NOT_CONNECTED);
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_ERR_NOT_CONNECTED, err, "Debe rechazar envio si la red no esta conectada");

    // 2. Probar protección contra puntero nulo (NULL)
    err = CellularNet_SendAlertFrame(NULL, sizeof(dummy_payload));
    printf("[TEST PARAM] -> Envio con payload NULL devuelve: %d (Esperado: %d para ERR_PARAM)\n", 
           err, CELL_NET_ERR_PARAM);
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_ERR_PARAM, err, "Debe rechazar envio con puntero nulo");

    // 3. Probar protección contra longitud cero
    err = CellularNet_SendAlertFrame(dummy_payload, 0);
    printf("[TEST PARAM] -> Envio con longitud 0 devuelve: %d (Esperado: %d para ERR_PARAM)\n", 
           err, CELL_NET_ERR_PARAM);
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_ERR_PARAM, err, "Debe rechazar envio con longitud cero");

    // 4. Probar protección contra desbordamiento de buffer (> MAX_PAYLOAD_SIZE)
    uint8_t oversized_payload[300] = {0};
    err = CellularNet_SendAlertFrame(oversized_payload, sizeof(oversized_payload));
    printf("[TEST PARAM] -> Envio con payload sobredimensionado devuelve: %d (Esperado: %d para ERR_PARAM)\n", 
           err, CELL_NET_ERR_PARAM);
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_ERR_PARAM, err, "Debe rechazar envio que supere el MAX_PAYLOAD_SIZE");

    printf("========================================\n");
    printf("[TEST PARAM] ¡Prueba de validacion de parametros superada con exito!\n");
    printf("========================================\n");
    
    TEST_PASS();
}
//////////////////
*/

/*

TEST_CASE("TEST-MW-NET-HW-INTEGRATION State Machine Power-On & UART Handshake", "[middleware3][cellular][hardware]") {
    printf("\n========================================\n");
    printf("[TEST MW HW] Iniciando middleware sobre hardware real...\n");
    printf("========================================\n");

    // 1. Capa BSP: Inicializamos el hardware físico primero
    //CellularModemInit();

    // 2. Capa Middleware: Inicializa colas y lanza la tarea de FreeRTOS
    cellular_net_err_t err = CellularNet_Init();
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_OK, err, "Fallo al inicializar el middleware de red");

    printf("[TEST MW HW] -> Monitoreando maquina de estados de la tarea de fondo...\n");

    int timeout_counter = 0;
    cellular_net_status_t status;
    bool reached_wait_rdy = false;

    // 3. Monitorear cómo la tarea de FreeRTOS toma el control, aprieta el PWRKEY y espera el "RDY"
    while (timeout_counter < 12) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        status = CellularNet_GetStatus();
        
        printf("[TEST MW HW] [t=%ds] -> Estado actual: %d | Errores: %d | RSSI: %d\n", 
               timeout_counter + 1, status.current_state, status.consecutive_errors, status.rssi);

        // Verificamos si la máquina de estados avanzó al menos hasta la espera de RDY o más allá
        if (status.current_state >= CELL_STATE_WAIT_RDY) {
            reached_wait_rdy = true;
            printf("[TEST MW HW] -> ¡Exito! La maquina de estados ejecuto el pulso y avanzo al estado: %d\n", status.current_state);
            break;
        }
        timeout_counter++;
    }

    TEST_ASSERT_TRUE_MESSAGE(reached_wait_rdy, "El middleware se quedo trabado en CELL_STATE_OFF o POWERING_ON");

    printf("========================================\n");
    printf("[TEST MW HW] ¡Prueba de integracion de middleware superada!\n");
    printf("========================================\n");
    
    TEST_PASS();
}

*/
/*
TEST_CASE("TEST-MW-NET-HW-INTEGRATION State Machine Power-On & UART Handshake", "[middleware3][cellular][hardware]") {
    printf("\n========================================\n");
    printf("[TEST MW HW] Iniciando middleware sobre hardware real...\n");
    printf("========================================\n");

    // 1. Capa BSP: Inicializamos el hardware físico primero
    CellularModemInit();

    // 2. Capa Middleware: Inicializa colas y lanza la tarea de FreeRTOS
    cellular_net_err_t err = CellularNet_Init();
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_OK, err, "Fallo al inicializar el middleware de red");

    printf("[TEST MW HW] -> Monitoreando booteo, recepcion de 'RDY' e inicio de comandos AT...\n");

    int timeout_counter = 0;
    cellular_net_status_t status;
    bool reached_init_at = false;

    // Damos un margen de hasta 15 segundos (el módem tarda unos segundos en bootear y enviar RDY)
    while (timeout_counter < 15) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        status = CellularNet_GetStatus();
        
        printf("[TEST MW HW] [t=%ds] -> Estado actual: %d | Errores: %d | RSSI: %d\n", 
               timeout_counter + 1, status.current_state, status.consecutive_errors, status.rssi);

        // Si llega al estado 3 (CELL_STATE_INIT_AT), significa que capturó el "RDY" exitosamente
        if (status.current_state >= 3) {
            reached_init_at = true;
            printf("[TEST MW HW] -> ¡Exito total! Se recibio el 'RDY' y avanzo a configuracion AT (Estado: %d)\n", status.current_state);
            break;
        }
        timeout_counter++;
    }

    TEST_ASSERT_TRUE_MESSAGE(reached_init_at, "El middleware se quedo trabado esperando el 'RDY' del modem");

    printf("========================================\n");
    printf("[TEST MW HW] ¡Prueba de integracion completa superada!\n");
    printf("========================================\n");
    
    TEST_PASS();
}

TEST_CASE("TEST-BSP-MODEM-05 Soft Power Off via AT+QPOWD", "[bsp][cellular_off][interactive]")
{
    printf("\n[BSP TEST] Iniciando apagado suave (Soft Power-Off) via comando AT...\n");

    // 1. Inicializar la capa física (UART y pines)
    CellularModemInit();

    // 2. Enviar el comando de apagado normal de Quectel (AT+QPOWD=1)
    // Este comando realiza un apagado seguro (desregistra de red, guarda estados y apaga la radio)
    const char *cmd = "AT+QPOWD=1\r\n";
    int written = CellularModemWriteRaw((const uint8_t *)cmd, strlen(cmd));
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, written, "Error al enviar comando AT+QPOWD hacia el módem");

    // 3. Leer la respuesta inmediata del módulo (suele retornar OK antes de iniciar la secuencia de apagado)
    uint8_t buffer[128] = {0};
    int read_bytes = CellularModemReadRaw(buffer, sizeof(buffer) - 1, 3000);
    
    if (read_bytes > 0) {
        printf("--> Respuesta a AT+QPOWD: %s\n", buffer);
    } else {
        printf("--> [INFO] Sin respuesta inmediata al comando de apagado.\n");
    }

    // 4. Dar tiempo para que el módem cierre archivos, desinscriba la red y apague los subsistemas internos
    printf("Esperando 5 segundos para que el módulo complete el apagado seguro...\n");
    for (int i = 5; i > 0; i--) {
        printf("Restan %d segundos...\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // 5. Verificar que el módem haya quedado completamente mudo intentando enviar un comando "AT"
    const char *test_cmd = "AT\r\n";
    CellularModemWriteRaw((const uint8_t *)test_cmd, strlen(test_cmd));
    
    memset(buffer, 0, sizeof(buffer));
    read_bytes = CellularModemReadRaw(buffer, sizeof(buffer) - 1, 1000);

    if (read_bytes <= 0 || strstr((char *)buffer, "OK") == NULL) {
        printf("--> ¡Éxito! El módem se apagó correctamente de forma suave (Soft Off verificado por UART).\n");
        TEST_PASS();
    } else {
        printf("--> [FALLO] El módem sigue respondiendo tras el comando de apagado: %s\n", buffer);
        TEST_FAIL_MESSAGE("El apagado suave (AT+QPOWD) no surtió efecto y sigue respondiendo.");
    }
}

*/
#include <stdio.h>

#include "unity.h"

#include "cellular_net.h"
#include "cellular_modem.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <stdint.h>



TEST_CASE("TEST-MW-CELLULAR-01 Network initialization","[cellular_net][middleware][interactive]")
{
    cellular_net_err_t result;
    cellular_net_status_t status;

    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-CELLULAR-01\n");
    printf(" Cellular Network Initialization\n");
    printf("========================================\n");


    /*
     * --------------------------------------------------------
     * 1. Inicializar Middleware
     * --------------------------------------------------------
     */

    printf("\n--> Inicializando CellularNet...\n");
    result = CellularNet_Init();
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_OK, result, "ERROR: CellularNet_Init() fallo");

    printf(
        "--> CellularNet_Init() OK\n"
    );


    /*
     * --------------------------------------------------------
     * 2. Esperar que la FSM avance
     * --------------------------------------------------------
     *
     * La tarea CellularNet_Task() fue creada por
     * CellularNet_Init().
     *
     * El test solamente observa su estado.
     */

    printf(
        "\n--> Esperando que la FSM inicialice "
        "el modem y la red...\n"
    );


    const uint32_t timeout_ms = 60000U;
    const uint32_t step_ms = 500U;

    uint32_t elapsed_ms = 0U;


    while (elapsed_ms < timeout_ms)
    {
        status = CellularNet_GetStatus();

        printf(
            "--> Estado actual: %d\n",
            status.current_state
        );

        /*
         * Si llegó a CONNECTED_IDLE,
         * la red está lista.
         */

        if (status.current_state ==
            CELL_STATE_CONNECTED_IDLE)
        {
            printf("\n========================================\n");
            printf( " CellularNet LISTO\n" );
            printf("========================================\n");
            TEST_ASSERT_TRUE(CellularNet_IsReady());
            return;
        }

        /*
         * Si entra en recuperación,
         * consideramos que el arranque falló.
         */

        if (status.current_state ==
            CELL_STATE_ERROR_RECOVERY)
        {
            TEST_FAIL_MESSAGE("ERROR: CellularNet entro en ERROR_RECOVERY");
        }
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        elapsed_ms += step_ms;
    }


    /*
     * --------------------------------------------------------
     * 3. Timeout general
     * --------------------------------------------------------
     */

    status = CellularNet_GetStatus();

    printf(
        "\n--> Timeout esperando CellularNet.\n"
    );

    printf(
        "--> Estado final: %d\n",
        status.current_state
    );

    TEST_FAIL_MESSAGE("ERROR: CellularNet no llego a CONNECTED_IDLE");
}


TEST_CASE("TEST-MW-CELLULAR-01 Network initialization","[apagar]")
{

    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-CELLULAR-0X\n");
    printf(" apagar\n");
    printf("========================================\n");

    CellularModemHardPowerOff();

}


TEST_CASE("TEST-MW-CELLULAR-prueba de fuego","[prueba_fuego]")
{
    char rx_buffer[512];
    int received;

    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-CELLULAR-PRUEBA DE FUEGO\n");
    printf(" Diagnostico de registro celular\n");
    printf("========================================\n");

    /*
     * Inicializar acceso al modem
     */
    printf("\n--> Inicializando modem...\n");

    TEST_ASSERT_TRUE(
        CellularModemInit()
    );

    printf("--> CellularModemInit() OK\n");

    /*
     * Encender modem
     */
    printf("\n--> Encendiendo modem...\n");

    CellularModemPowerPulse();

    /*
     * Esperar RDY
     */
    printf("--> Esperando RDY...\n");

    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    received = CellularModemRead(
        (uint8_t *)rx_buffer,
        sizeof(rx_buffer) - 1U,
        10000U
    );

    TEST_ASSERT_GREATER_THAN(
        0,
        received
    );

    rx_buffer[received] = '\0';

    printf(
        "\n[MODEM BOOT]\n%s\n",
        rx_buffer
    );

    TEST_ASSERT_NOT_NULL(
        strstr(rx_buffer, "RDY")
    );

    printf("--> RDY recibido.\n");

    /*
     * ATE0
     */
    printf("\n========================================\n");
    printf(" AT INICIALIZACION\n");
    printf("========================================\n");

    CellularModemWrite(
        (const uint8_t *)"ATE0\r\n",
        strlen("ATE0\r\n")
    );

    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    received = CellularModemRead(
        (uint8_t *)rx_buffer,
        sizeof(rx_buffer) - 1U,
        3000U
    );

    if (received > 0)
    {
        rx_buffer[received] = '\0';

        printf(
            "\n[ATE0]\n%s\n",
            rx_buffer
        );
    }

    /*
     * AT+CSQ
     */
    printf("\n========================================\n");
    printf(" CONSULTA AT+CSQ\n");
    printf("========================================\n");

    CellularModemWrite(
        (const uint8_t *)"AT+CSQ\r\n",
        strlen("AT+CSQ\r\n")
    );

    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    received = CellularModemRead(
        (uint8_t *)rx_buffer,
        sizeof(rx_buffer) - 1U,
        3000U
    );

    if (received > 0)
    {
        rx_buffer[received] = '\0';

        printf(
            "\n[AT+CSQ]\n%s\n",
            rx_buffer
        );
    }
    else
    {
        printf(
            "[AT+CSQ] SIN RESPUESTA\n"
        );
    }

    /*
     * AT+CEREG?
     */
    printf("\n========================================\n");
    printf(" CONSULTA AT+CEREG?\n");
    printf("========================================\n");

    CellularModemWrite(
        (const uint8_t *)"AT+CEREG?\r\n",
        strlen("AT+CEREG?\r\n")
    );

    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    received = CellularModemRead(
        (uint8_t *)rx_buffer,
        sizeof(rx_buffer) - 1U,
        3000U
    );

    if (received > 0)
    {
        rx_buffer[received] = '\0';

        printf(
            "\n[AT+CEREG?]\n%s\n",
            rx_buffer
        );
    }
    else
    {
        printf(
            "[AT+CEREG?] SIN RESPUESTA\n"
        );
    }

    /*
     * AT+CGREG?
     */
    printf("\n========================================\n");
    printf(" CONSULTA AT+CGREG?\n");
    printf("========================================\n");

    CellularModemWrite(
        (const uint8_t *)"AT+CGREG?\r\n",
        strlen("AT+CGREG?\r\n")
    );

    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    received = CellularModemRead(
        (uint8_t *)rx_buffer,
        sizeof(rx_buffer) - 1U,
        3000U
    );

    if (received > 0)
    {
        rx_buffer[received] = '\0';

        printf(
            "\n[AT+CGREG?]\n%s\n",
            rx_buffer
        );
    }
    else
    {
        printf(
            "[AT+CGREG?] SIN RESPUESTA\n"
        );
    }

    /*
     * AT+COPS?
     */
    printf("\n========================================\n");
    printf(" CONSULTA AT+COPS?\n");
    printf("========================================\n");

    CellularModemWrite(
        (const uint8_t *)"AT+COPS?\r\n",
        strlen("AT+COPS?\r\n")
    );

    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    received = CellularModemRead(
        (uint8_t *)rx_buffer,
        sizeof(rx_buffer) - 1U,
        5000U
    );

    if (received > 0)
    {
        rx_buffer[received] = '\0';

        printf(
            "\n[AT+COPS?]\n%s\n",
            rx_buffer
        );
    }
    else
    {
        printf(
            "[AT+COPS?] SIN RESPUESTA\n"
        );
    }

    /*
     * Fin del diagnóstico
     */
    printf("\n========================================\n");
    printf(" FIN PRUEBA DE FUEGO\n");
    printf("========================================\n");

    TEST_PASS();
}




