#include "cellular_net.h"
#include "cellular_modem.h"
#include "event_frame.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 *  Configuración de red y servidor
 * ============================================================================ */
#define APN_NAME                "datos.personal.com"
#define APN_USER                "datos"
#define APN_PASS                "datos"

#define SERVER_IP               "xbkcp-190-183-23-94.run.pinggy-free.link"
#define SERVER_PORT             45241
#define SOCKET_PROTO            "TCP"

#define QUEUE_LENGTH            5
#define MAX_CONSECUTIVE_ERRORS  3
#define DEFAULT_FALLBACK_IMEI   "123456789012345"

/* ============================================================================
 *  Tipos y Estructuras Internas
 * ============================================================================ */
typedef struct {
    uint8_t payload[MAX_ALERT_PAYLOAD_SIZE];
    uint16_t length;
} alert_event_t;

static cellular_net_status_t g_net_status = {
    .state = CELL_STATE_OFF,
    .rssi = 99,
    .consecutive_errors = 0,
    .uptime_seconds = 0
};

static QueueHandle_t g_alert_queue = NULL;
static uint32_t g_last_tick_sec = 0;

/* Storage seguro de IMEI dentro de la tarea de red (evita colisión de UART) */
static char g_modem_imei[16] = DEFAULT_FALLBACK_IMEI;
static bool g_imei_read_ok = false;

/* ============================================================================
 *  FSM Interna del Módem Celular
 * ============================================================================ */
static void CellularNet_FsmStep(void) {
    // 1. Cronómetro de tiempo activo (Uptime)
    uint32_t current_tick_sec = xTaskGetTickCount() / configTICK_RATE_HZ;
    if (g_net_status.state == CELL_STATE_READY && current_tick_sec != g_last_tick_sec) {
        g_net_status.uptime_seconds++;
        g_last_tick_sec = current_tick_sec;
    }

    // 2. Máquina de Estados Finitos (FSM)
    switch (g_net_status.state) {

    case CELL_STATE_STARTING:
        printf("[CELL_NET] Iniciando verificación de arranque del módem...\n");

        // Paso A: Probar si el módem ya está encendido y respondiendo AT
        for (int i = 0; i < 3; i++) {
            if (CellularModemIsReady()) {
                printf("[CELL_NET] ¡Módem detectado encendido! Pasando a CONNECTING...\n");
                g_net_status.state = CELL_STATE_CONNECTING;
                g_net_status.consecutive_errors = 0;
                return;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // Paso B: Si no responde AT, aplicar 1 pulso de encendido PWRKEY
        printf("[CELL_NET] Módem no responde AT. Aplicando pulso de encendido PWRKEY...\n");
        CellularModemPowerPulse();

        // Paso C: Esperar inicialización del módem y respuesta AT
        printf("[CELL_NET] Esperando respuesta del módem (hasta 6s)...\n");
        if (CellularModemWaitBoot(6000) || CellularModemIsReady()) {
            printf("[CELL_NET] Módem respondió AT tras PWRKEY. Pasando a CONNECTING...\n");
            g_net_status.state = CELL_STATE_CONNECTING;
            g_net_status.consecutive_errors = 0;
        } else {
            printf("[CELL_NET] [ADVERTENCIA] Módem sin respuesta tras PWRKEY.\n");
            g_net_status.consecutive_errors++;
            g_net_status.state = CELL_STATE_ERROR;
        }
        break;

    case CELL_STATE_CONNECTING:
        if (!CellularModemIsSimReady() || !CellularModemIsNetworkRegistered()) {
            printf("[CELL_NET] Esperando SIM / Registro en Red LTE (+CEREG)...\n");
            break;
        }

        // Carga del IMEI desde la única tarea con acceso seguro al UART
        if (!g_imei_read_ok) {
            if (CellularModemGetIMEI(g_modem_imei, sizeof(g_modem_imei))) {
                g_imei_read_ok = true;
                printf("[CELL_NET] IMEI real leído con éxito del hardware: %s\n", g_modem_imei);
            }
        }

        // Chequeo de idempotencia ante resets del MCU
        if (CellularModemIsPdpActive(NULL, 0)) {
            printf("[CELL_NET] Contexto PDP ya activo. Red en READY.\n");
            g_net_status.state = CELL_STATE_READY;
            g_net_status.consecutive_errors = 0;
            break;
        }

        printf("[CELL_NET] Configurando y activando PDP context...\n");
        if (CellularModemConfigurePdp(APN_NAME, APN_USER, APN_PASS) &&
            CellularModemActivatePdp()) {

            printf("[CELL_NET] ¡PDP activado con éxito! Red en CELL_STATE_READY.\n");
            g_net_status.state = CELL_STATE_READY;
            g_net_status.consecutive_errors = 0;
        } else {
            printf("[CELL_NET] [ERROR] Falló activación de PDP context.\n");
            g_net_status.consecutive_errors++;
            g_net_status.state = CELL_STATE_ERROR;
        }
        break;

    case CELL_STATE_READY:
        // Re-intento de lectura si por alguna razón no se obtuvo en CONNECTING
        if (!g_imei_read_ok) {
            if (CellularModemGetIMEI(g_modem_imei, sizeof(g_modem_imei))) {
                g_imei_read_ok = true;
                printf("[CELL_NET] IMEI real leído con éxito en READY: %s\n", g_modem_imei);
            }
        }

        if (!CellularModemIsPdpActive(NULL, 0)) {
            printf("[CELL_NET] [ALERTA] Pérdida de contexto PDP. Reconectando...\n");
            g_net_status.state = CELL_STATE_CONNECTING;
        }
        break;

    case CELL_STATE_ERROR:
        printf("[CELL_NET] En estado de error (reintentos: %u). Pausa pasiva de 5s...\n", g_net_status.consecutive_errors);
        vTaskDelay(pdMS_TO_TICKS(5000));
        // Volver a STARTING donde primero probará AT sin forzar apagado duro
        g_net_status.state = CELL_STATE_STARTING;
        break;

    default:
        break;
    }
}

/* ============================================================================
 *  Transmisión de Trama por Socket TCP
 * ============================================================================ */
static bool CellularNet_TransmitFrame(const uint8_t *payload, uint16_t length) {
    bool tx_ok = false;

    if (CellularModemSocketOpen(SOCKET_PROTO, SERVER_IP, SERVER_PORT)) {
        if (CellularModemSocketSend(payload, length)) {
            uint8_t rx_buffer[64];
            uint16_t rx_bytes = 0;

            if (CellularModemSocketReceive(rx_buffer, sizeof(rx_buffer) - 1, &rx_bytes) && rx_bytes > 0) {
                rx_buffer[rx_bytes] = '\0';
                tx_ok = true;
            } else {
                tx_ok = true; // Confirmación implícita tras Send OK
            }
        }
        CellularModemSocketClose();
    }
    return tx_ok;
}

/* ============================================================================
 *  Tarea de FreeRTOS orientada a Eventos
 * ============================================================================ */
static void CellularNet_TaskRoutine(void *pvParameters) {
    (void)pvParameters;
    alert_event_t event;

    for (;;) {
        // 1. Ejecutamos un paso de la FSM
        CellularNet_FsmStep();

        // 2. Si la red está lista, procesamos las alertas de la cola
        if (g_net_status.state == CELL_STATE_READY) {
            if (xQueueReceive(g_alert_queue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
                if (!CellularNet_TransmitFrame(event.payload, event.length)) {
                    g_net_status.consecutive_errors++;
                    
                    if (g_net_status.consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
                        g_net_status.state = CELL_STATE_ERROR;
                    }
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ============================================================================
 *  API Pública del Módulo Cellular Net
 * ============================================================================ */
cellular_net_err_t CellularNet_Init(void) {
    if (g_net_status.state != CELL_STATE_OFF && g_alert_queue != NULL) {
        return CELL_NET_OK;
    }

    if (!CellularModemInit()) {
        g_net_status.state = CELL_STATE_ERROR;
        return CELL_NET_ERR_NOT_READY;
    }

    g_alert_queue = xQueueCreate(QUEUE_LENGTH, sizeof(alert_event_t));
    if (g_alert_queue == NULL) {
        return CELL_NET_ERR_NOT_READY;
    }

    g_net_status.state = CELL_STATE_STARTING;

    xTaskCreate(CellularNet_TaskRoutine, "cell_net_task", 4096, NULL, 5, NULL);

    return CELL_NET_OK;
}

cellular_net_err_t CellularNet_SendAlertFrame(const uint8_t *payload, uint16_t length) {
    if (payload == NULL || length == 0 || length > MAX_ALERT_PAYLOAD_SIZE) {
        return CELL_NET_ERR_PARAM;
    }

    if (g_net_status.state != CELL_STATE_READY) {
        return CELL_NET_ERR_NOT_READY;
    }

    alert_event_t event;
    memcpy(event.payload, payload, length);
    event.length = length;

    if (xQueueSend(g_alert_queue, &event, 0) != pdTRUE) {
        return CELL_NET_ERR_BUSY;
    }

    return CELL_NET_OK;
}

cellular_net_status_t CellularNet_GetStatus(void) {
    return g_net_status;
}

bool CellularNet_IsReady(void) {
    return (g_net_status.state == CELL_STATE_READY);
}

/* 🟢 Obtención segura de IMEI expuesta por el módulo de red */
bool CellularNet_GetIMEI(char *imei_out, size_t max_len) {
    if (imei_out == NULL || max_len < 16) {
        return false;
    }
    strncpy(imei_out, g_modem_imei, 15);
    imei_out[15] = '\0';
    return g_imei_read_ok;
}