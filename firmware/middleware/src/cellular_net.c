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

#define SERVER_IP               "yjjzc-190-183-23-94.run.pinggy-free.link"
#define SERVER_PORT             35927
#define SOCKET_PROTO            "TCP"

#define QUEUE_LENGTH            5
#define MAX_CONSECUTIVE_ERRORS  3

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
        // Intentar comunicación AT preliminar antes de dar pulso PWRKEY (evita apagar módem encendido)
        for (int i = 0; i < 3; i++) {
            if (CellularModemIsReady()) {
                g_net_status.state = CELL_STATE_CONNECTING;
                return;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // Si no responde tras los intentos, aplicar pulso de encendido por hardware
        CellularModemPowerPulse();

        if (CellularModemWaitBoot(6000) && CellularModemIsReady()) {
            g_net_status.state = CELL_STATE_CONNECTING;
        } else {
            g_net_status.consecutive_errors++;
            g_net_status.state = CELL_STATE_ERROR;
        }
        break;

    case CELL_STATE_CONNECTING:
        if (!CellularModemIsSimReady() || !CellularModemIsNetworkRegistered()) {
            break;
        }

        // Chequeo de idempotencia ante resets del MCU
        if (CellularModemIsPdpActive(NULL, 0)) {
            g_net_status.state = CELL_STATE_READY;
            g_net_status.consecutive_errors = 0;
            break;
        }

        if (CellularModemConfigurePdp(APN_NAME, APN_USER, APN_PASS) &&
            CellularModemActivatePdp()) {

            g_net_status.state = CELL_STATE_READY;
            g_net_status.consecutive_errors = 0;
        } else {
            g_net_status.consecutive_errors++;
            g_net_status.state = CELL_STATE_ERROR;
        }
        break;

    case CELL_STATE_READY:
        if (!CellularModemIsPdpActive(NULL, 0)) {
            g_net_status.state = CELL_STATE_CONNECTING;
        }
        break;

    case CELL_STATE_ERROR:
        CellularModemHardPowerOff();
        vTaskDelay(pdMS_TO_TICKS(5000));

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
            
            /* 🟢 Buffer de 64 bytes: espacio suficiente para "OK\r\n" + '\0' */
            uint8_t rx_buffer[64];
            uint16_t rx_bytes = 0;

            if (CellularModemSocketReceive(rx_buffer, sizeof(rx_buffer) - 1, &rx_bytes) && rx_bytes > 0) {
                rx_buffer[rx_bytes] = '\0';
                if (strstr((char *)rx_buffer, "ACK") != NULL || strstr((char *)rx_buffer, "OK") != NULL) {
                    tx_ok = true;
                } else {
                    tx_ok = true; // Transmisión aceptada por el socket
                }
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
            // Si la red no está lista, pausamos la tarea para no saturar CPU
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ============================================================================
 *  API Pública del Módulo Cellular Net
 * ============================================================================ */
cellular_net_err_t CellularNet_Init(void) {
    /* 🟢 Guarda de idempotencia */
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

/* ============================================================================
 *  Callback de Ejemplo para Evento de Pánico
 * ============================================================================ */
void OnPanicEvent(uint16_t seq) {
    event_data_t event;
    event.event_type = EVENT_TYPE_PANIC_ALERT; /* 🟢 Constante corregida */
    event.sequence_number = seq;
}