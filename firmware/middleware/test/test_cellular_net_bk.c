#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "cellular_net.h"
#include "cellular_modem.h" /* Incluido para llamar a las funciones del hardware */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

TEST_CASE("TEST-NET-01 Inicializacion y ciclo de vida de la FSM", "[cellular_net1][init]")
{
    printf("\n========================================\n");
    printf(" TEST CELLULAR_NET: Inicializacion y FSM\n");
    printf("========================================\n");

    /* --------------------------------------------------------------------
     * ETAPA 1: APAGADO
     * -------------------------------------------------------------------- */
    printf("[ETAPA 1] Enviando pulso de apagado duro (Hard Off)...\n");
    CellularModemHardPowerOff(); /* Genera el pulso de 5s en PWRKEY */

    /* --------------------------------------------------------------------
     * ETAPA 2: REPOSO PROLONGADO CON EL MÓDEM APAGADO (20 SEGUNDOS)
     * Da tiempo a que los capacitores internos del Quectel se descarguen 
     * completamente y el PMIC quede en reposo absoluto.
     * -------------------------------------------------------------------- */
    printf("\n[ETAPA 2] MÓDEM APAGADO. Iniciando espera de reposo (20 segundos)...\n");
    for (int i = 20; i > 0; i--) {
        printf(" -> Reposo con módem apagado: quedan %2d segundos...\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf(" -> Reposo finalizado. El PMIC está desenergizado y listo para encender.\n\n");

    /* --------------------------------------------------------------------
     * ETAPA 3: RECIÉN AHORA ARRANCA LA FSM (ENCENDIDO Y ESPERA DE READY)
     * -------------------------------------------------------------------- */
    printf("[ETAPA 3] Lanzando FSM con CellularNet_Init()...\n");
    cellular_net_err_t err = CellularNet_Init();
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_OK, err, "FAIL: Error al inicializar middleware cellular_net");

    printf("[ETAPA 3.1] Esperando que la FSM encienda el módem y alcance CELL_STATE_READY (60s max)...\n");
    
    bool is_ready = false;
    for (int timeout = 0; timeout < 60; timeout++) {
        cellular_net_status_t status = CellularNet_GetStatus();
        
        printf(" -> Status: %d | Errores: %d | Uptime: %luseg\n", 
               status.state, status.consecutive_errors, (unsigned long)status.uptime_seconds);

        if (CellularNet_IsReady()) {
            is_ready = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    TEST_ASSERT_TRUE_MESSAGE(is_ready, "FAIL: La FSM no alcanzo el estado READY en el tiempo limite");
    
    printf("\n========================================\n");
    printf(" ¡FSM EN CELL_STATE_READY CORRECTAMENTE!\n");
    printf("========================================\n");
}


TEST_CASE("TEST-NET-02 Envío nominal de trama de alerta", "[cellular_net2][send]")
{
    printf("\n========================================\n");
    printf(" TEST CELLULAR_NET: Encolado de Alerta\n");
    printf("========================================\n");

    /* Paso 1: Verificar que la red se mantenga en READY desde el Test 1 */
    TEST_ASSERT_TRUE_MESSAGE(CellularNet_IsReady(), "FAIL: La red debe estar en READY para ejecutar este test");

    /* Trama de prueba de 5 bytes */
    uint8_t alert_payload[] = {0xAA, 0x01, 0x02, 0x03, 0xFF}; 
    
    printf("[PASO 1] Encolando trama de %d bytes...\n", sizeof(alert_payload));
    cellular_net_err_t err = CellularNet_SendAlertFrame(alert_payload, sizeof(alert_payload));
    
    TEST_ASSERT_EQUAL_MESSAGE(CELL_NET_OK, err, "FAIL: Error al intentar encolar la alerta en la cola FreeRTOS");

    /* Paso 2: Pausa para que la tarea de fondo abra el socket TCP, envíe y cierre */
    printf("[PASO 2] Esperando que la tarea procese el socket TCP a través del túnel (5s)...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    /* Paso 3: Confirmación de entrega sin errores de socket acumulados */
    cellular_net_status_t status = CellularNet_GetStatus();
    TEST_ASSERT_EQUAL_MESSAGE(0, status.consecutive_errors, "FAIL: La transmisión por socket genero errores consecutivos");

    printf("\n========================================\n");
    printf(" ¡TRAMA ENTREGADA AL SOCKET EXITOSAMENTE!\n");
    printf("========================================\n");
}

/*
gabriel@nnzgab:~$ python3 -c "import socket; s=socket.socket(); s.bind(('0.0.0.0', 8089)); s.listen(1); print('Escuchando en 8089...'); conn, addr = s.accept(); print('Conectado desde:', addr); data = conn.recv(1024); print('HEX:', ' '.join(f'{b:02X}' for b in data))"
Escuchando en 8089...
Conectado desde: ('127.0.0.1', 42788)
HEX: AA 01 02 03 FF
gabriel@nnzgab:~$ 

*/



TEST_CASE("TEST-NET-03 Pruebas defensivas de parámetros en API pública", "[cellular_net3][defensive]")
{
    printf("\n========================================\n");
    printf(" TEST CELLULAR_NET: Validaciones defensivas\n");
    printf("========================================\n");

    uint8_t dummy_buf[10] = {0};

    /* Caso A: Puntero NULL */
    printf("[TEST A] Probando con puntero NULL...\n");
    TEST_ASSERT_EQUAL(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(NULL, 5));

    /* Caso B: Longitud Cero */
    printf("[TEST B] Probando con longitud cero...\n");
    TEST_ASSERT_EQUAL(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(dummy_buf, 0));

    /* Caso C: Payload excede el tamaño máximo permitido */
    printf("[TEST C] Probando con tamaño sobredimensionado (> MAX_ALERT_PAYLOAD_SIZE)...\n");
    uint8_t oversized_buf[MAX_ALERT_PAYLOAD_SIZE + 10] = {0};
    TEST_ASSERT_EQUAL(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(oversized_buf, sizeof(oversized_buf)));

    printf("\n========================================\n");
    printf(" ¡VALIDACIONES DEFENSIVAS REBOTARON OK!\n");
    printf("========================================\n");
}

/*
 * TODO: TEST-NET-04 Pruebas de desborde y teardown de recursos (Pendiente)
 * - Encolar 6 alertas seguidas sin pausa para forzar retorno CELL_NET_ERR_BUSY.
 * - Intentar enviar alerta en estado OFF o STARTING para forzar CELL_NET_ERR_NOT_READY.
 * - Ejecutar CellularNet_DeInit() y verificar que g_net_task_handle y g_alert_queue se destruyan limpiamente.
 */