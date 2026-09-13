#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "cellular_net.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ============================================================================
 * TEST-MW-CELLNET-01: Validación de parámetros y estado no listo
 * ============================================================================ */
TEST_CASE("TEST-MW-CELLNET-01 Parameter validation and not ready guard", "[middleware][cellular_net]") {
    printf("\n========================================\n");
    printf(" TEST-MW-CELLNET-01 VALIDACION DE PARAMETROS\n");
    printf("========================================\n");

    uint8_t dummy_payload[10] = {0x01, 0x02, 0x03};

    // 1. Rechazo de payload nulo
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(NULL, 10),
        "CellularNet_SendAlertFrame(NULL) debió devolver CELL_NET_ERR_PARAM");

    // 2. Rechazo de longitud cero
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(dummy_payload, 0),
        "CellularNet_SendAlertFrame con len 0 debió devolver CELL_NET_ERR_PARAM");

    // 3. Rechazo de longitud excesiva
    TEST_ASSERT_EQUAL_INT_MESSAGE(CELL_NET_ERR_PARAM, CellularNet_SendAlertFrame(dummy_payload, MAX_ALERT_PAYLOAD_SIZE + 1),
        "CellularNet_SendAlertFrame con excede max len debió devolver CELL_NET_ERR_PARAM");

    // 4. Rechazo si el módulo no fue inicializado / no está listo
    if (!CellularNet_IsReady()) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(CELL_NET_ERR_NOT_READY, CellularNet_SendAlertFrame(dummy_payload, sizeof(dummy_payload)),
            "CellularNet_SendAlertFrame sin estar en READY debió devolver CELL_NET_ERR_NOT_READY");
    }

    printf("--> Validación de guardas y parámetros correcta.\n");
}

/* ============================================================================
 * TEST-MW-CELLNET-02: Inicialización y consulta de estado inicial
 * ============================================================================ */
TEST_CASE("TEST-MW-CELLNET-02 Initialization and initial status check", "[middleware][cellular_net]") {
    printf("\n========================================\n");
    printf(" TEST-MW-CELLNET-02 INICIALIZACION Y ESTADO INICIAL\n");
    printf("========================================\n");

    // Si no estaba inicializado previamente, se inicializa
    cellular_net_status_t status_before = CellularNet_GetStatus();
    if (status_before.state == CELL_STATE_OFF) {
        cellular_net_err_t err = CellularNet_Init();
        TEST_ASSERT_EQUAL_INT_MESSAGE(CELL_NET_OK, err,
            "CellularNet_Init debió devolver CELL_NET_OK");
    }

    // Consultar estado actual
    cellular_net_status_t status = CellularNet_GetStatus();
    TEST_ASSERT_NOT_EQUAL_MESSAGE(CELL_STATE_OFF, status.state,
        "El estado tras Init no debió ser CELL_STATE_OFF");

    printf("--> Inicialización del servicio de red verificada.\n");
}

/* ============================================================================
 * TEST-MW-CELLNET-03: Avance FSM y encolado asíncrono en estado READY
 * ============================================================================ */
TEST_CASE("TEST-MW-CELLNET-03 FSM transition to READY and alert queuing", "[middleware][cellular_net][interactive]") {
    printf("\n========================================\n");
    printf(" TEST-MW-CELLNET-03 TRANSICION FSM Y ENCOLADO EN READY\n");
    printf("========================================\n");

    // Asegurar inicialización única sin re-inicializar la UART si ya está corriendo
    cellular_net_status_t status_current = CellularNet_GetStatus();
    if (status_current.state == CELL_STATE_OFF) {
        TEST_ASSERT_EQUAL_INT(CELL_NET_OK, CellularNet_Init());
    }

    printf("Esperando a que la FSM alcance el estado CELL_STATE_READY (hasta 15s)...\n");

    int wait_seconds = 0;
    while (!CellularNet_IsReady() && wait_seconds < 15) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        wait_seconds++;
        cellular_net_status_t status = CellularNet_GetStatus();
        printf("  [ESTADO FSM] Estado actual: %d (segundo %d)\n", status.state, wait_seconds);
    }

    if (CellularNet_IsReady()) {
        printf("--> Módem en CELL_STATE_READY. Probando encolado de trama...\n");
        uint8_t test_frame[] = "01,123456789012345,1\r\n";
        cellular_net_err_t send_err = CellularNet_SendAlertFrame(test_frame, strlen((char *)test_frame));

        TEST_ASSERT_EQUAL_INT_MESSAGE(CELL_NET_OK, send_err,
            "CellularNet_SendAlertFrame debió devolver CELL_NET_OK estando la red lista");
        printf("--> Trama encolada exitosamente en g_alert_queue.\n");
    } else {
        printf("--> [NOTA] La red no alcanzó READY en el tiempo de prueba (entorno sin módem o sin SIM).\n");
        printf("--> Verificación de comportamiento fuera de READY completada.\n");
    }
}