#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Capa Middleware */
#include "panic_handler.h"
#include "status_indicator.h"
#include "event_frame.h"
#include "cellular_net.h"
#include "cellular_modem.h"

/* Configuración de la aplicación */
#define PANIC_LED_HOLD_TIME_MS  2000U
#define DEFAULT_IMEI            "123456789012345"

/* Variables globales de la aplicación */
static char g_device_imei[16] = DEFAULT_IMEI;
static uint32_t g_panic_led_timer_ms = 0;
static bool g_panic_led_active = false;

/* ============================================================================
 * Callback de Evento de Pánico (Invocado por panic_handler tras debounce)
 * ============================================================================ */
static void OnPanicEvent(uint16_t sequence_number) {
    printf("\n========================================\n");
    printf(" ¡ALERTA DE PANICO DETECTADA! (Secuencia: %u)\n", sequence_number);
    printf("========================================\n");

    /* 1. Activar indicación visual de pánico */
    StatusIndicator_SetPanic(true);
    g_panic_led_active = true;
    g_panic_led_timer_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    /* 2. Preparar los datos de la estructura de evento */
    event_data_t event;
    event.event_type = 0x01; // Tipo: Alerta de Pánico
    event.sequence_number = sequence_number;
    strncpy(event.imei, g_device_imei, sizeof(event.imei) - 1);
    event.imei[sizeof(event.imei) - 1] = '\0';

    /* 3. Empaquetar la trama en formato texto CSV (TIPO,IMEI,SECUENCIA\r\n) */
    char frame_buffer[MAX_ALERT_PAYLOAD_SIZE];
    uint16_t frame_len = 0;

    event_frame_err_t frame_err = EventFrame_PackText(&event, frame_buffer, sizeof(frame_buffer), &frame_len);

    if (frame_err == EVENT_FRAME_OK) {
        printf("--> Trama generada [%u bytes]: %s", frame_len, frame_buffer);

        /* 4. Enviar trama a la cola del servicio de red celular */
        cellular_net_err_t net_err = CellularNet_SendAlertFrame((const uint8_t *)frame_buffer, frame_len);

        if (net_err == CELL_NET_OK) {
            printf("--> Trama de pánico depositada exitosamente en la cola de red.\n");
        } else if (net_err == CELL_NET_ERR_NOT_READY) {
            printf("--> [ADVERTENCIA] Red no lista. Trama descartada/rechazada.\n");
        } else if (net_err == CELL_NET_ERR_BUSY) {
            printf("--> [ERROR] Cola de red llena. Transmisión saturada.\n");
        } else {
            printf("--> [ERROR] Fallo al despachar trama a la red (Error: %d).\n", net_err);
        }
    } else {
        printf("--> [ERROR] Fallo al empaquetar la trama (Error: %d).\n", frame_err);
    }
}

/* ============================================================================
 * Mapeo de Estados: CellularNet FSM -> StatusIndicator LED Pattern
 * ============================================================================ */
static void UpdateStatusLedFromNetwork(void) {
    cellular_net_status_t net_status = CellularNet_GetStatus();

    switch (net_status.state) {
        case CELL_STATE_OFF:
            StatusIndicator_SetCellular(CELLULAR_STATUS_OFF);
            break;

        case CELL_STATE_STARTING:
            StatusIndicator_SetCellular(CELLULAR_STATUS_STARTING);
            break;

        case CELL_STATE_CONNECTING:
            StatusIndicator_SetCellular(CELLULAR_STATUS_SEARCHING);
            break;

        case CELL_STATE_READY:
            StatusIndicator_SetCellular(CELLULAR_STATUS_READY);
            break;

        case CELL_STATE_ERROR:
            StatusIndicator_SetCellular(CELLULAR_STATUS_OFF);
            break;

        default:
            StatusIndicator_SetCellular(CELLULAR_STATUS_OFF);
            break;
    }
}

/* ============================================================================
 * Bucle Principal de la Aplicación (ESP32 app_main)
 * ============================================================================ */
void app_main(void) {
    printf("\n==================================================\n");
    printf(" INICIALIZANDO SISTEMA DE ALERTA DE PANICO CELULAR\n");
    printf("==================================================\n");

    /* 1. Inicialización del Indicador de Estado (LEDs) */
    if (StatusIndicator_Init()) {
        printf("[OK] StatusIndicator inicializado correctamente.\n");
    } else {
        printf("[ERROR] Fallo al inicializar StatusIndicator.\n");
    }

    /* 2. Inicialización del Handler del Botón de Pánico */
    panic_handler_err_t panic_err = PanicHandler_Init(OnPanicEvent);
    if (panic_err == PANIC_HANDLER_OK) {
        printf("[OK] PanicHandler inicializado con callback registrado.\n");
    } else {
        printf("[ERROR] Fallo al inicializar PanicHandler (Error: %d).\n", panic_err);
    }

    /* 3. Inicialización del Servicio de Red Celular (FSM + Queue + Task) */
    cellular_net_err_t net_err = CellularNet_Init();
    if (net_err == CELL_NET_OK) {
        printf("[OK] CellularNet inicializado y tarea de red iniciada.\n");
    } else {
        printf("[ERROR] Fallo al inicializar CellularNet (Error: %d).\n", net_err);
    }

    /* 4. Intentar obtener el IMEI real del módem (opcional/best effort) */
    vTaskDelay(pdMS_TO_TICKS(500)); // Pequeña espera para estabilización
    if (CellularModemGetIMEI(g_device_imei, sizeof(g_device_imei))) {
        printf("[OK] IMEI obtenido del módem: %s\n", g_device_imei);
    } else {
        printf("[INFO] No se pudo leer IMEI del módem. Usando ID por defecto: %s\n", g_device_imei);
    }

    printf("\n==================================================\n");
    printf(" SISTEMA OPERATIVO Y EN BUCLE DE SUPERVISION\n");
    printf(" Presione el botón de pánico para emitir alertas.\n");
    printf("==================================================\n\n");

    /* 5. Bucle Principal de Ejecución No Bloqueante */
    for (;;) {
        /* Pasada de la FSM de detección de pulsación y debounce */
        PanicHandler_RunStep();

        /* Pasada del temporizador y parpadeo de LEDs de estado */
        StatusIndicator_RunStep();

        /* Actualización del patrón LED según el estado de la red celular */
        UpdateStatusLedFromNetwork();

        /* Gestión del temporizador de apagado del LED de pánico */
        if (g_panic_led_active) {
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if ((now - g_panic_led_timer_ms) >= PANIC_LED_HOLD_TIME_MS) {
                StatusIndicator_SetPanic(false);
                g_panic_led_active = false;
            }
        }

        /* Liberar tiempo de CPU para la tarea de red y el sistema */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}