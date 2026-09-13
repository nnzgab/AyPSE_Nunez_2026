#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Capa Middleware */
#include "panic_handler.h"
#include "status_indicator.h"
#include "event_frame.h"
#include "cellular_net.h"

/* Configuración de temporización visual */
#define PANIC_LED_HOLD_TIME_MS  2000U
#define STABILIZATION_WAIT_SEC  5U

/* Estado del LED de Pánico */
static uint32_t s_panic_led_timer_ms = 0;
static bool     s_panic_led_active   = false;
static bool     s_ready_announced    = false;

/* ============================================================================
 * Callback de Evento de Pánico (Invocado por panic_handler tras debounce)
 * ============================================================================ */
static void OnPanicEvent(uint16_t sequence_number) {
    printf("\n========================================\n");
    printf(" ¡ALERTA DE PANICO DETECTADA! (Secuencia: %u)\n", sequence_number);
    printf("========================================\n");

    /* 1. Activar indicación visual de pánico y temporizador */
    StatusIndicator_SetPanic(true);
    s_panic_led_active = true;
    s_panic_led_timer_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    /* 2. Preparar los datos de la estructura de evento */
    event_data_t event;
    event.event_type = EVENT_TYPE_PANIC_ALERT;
    event.sequence_number = sequence_number;

    /* 3. Obtener el IMEI real leído por cellular_net (o fallback seguro) */
    CellularNet_GetIMEI(event.imei, sizeof(event.imei));

    /* 4. Empaquetar la trama en formato texto CSV (TIPO,IMEI,SECUENCIA\r\n) */
    char frame_buffer[MAX_ALERT_PAYLOAD_SIZE];
    uint16_t frame_len = 0;

    event_frame_err_t frame_err = EventFrame_PackText(&event, frame_buffer, sizeof(frame_buffer), &frame_len);

    if (frame_err == EVENT_FRAME_OK) {
        printf("--> Trama generada [%u bytes]: %s", frame_len, frame_buffer);

        /* 5. Enviar trama a la cola del servicio de red celular */
        cellular_net_err_t net_err = CellularNet_SendAlertFrame((const uint8_t *)frame_buffer, frame_len);

        if (net_err == CELL_NET_OK) {
            printf("--> Trama de pánico depositada exitosamente en la cola de red.\n");
        } else if (net_err == CELL_NET_ERR_NOT_READY) {
            printf("--> [ADVERTENCIA] Red celular aún no está lista. Trama rechazada.\n");
        } else if (net_err == CELL_NET_ERR_BUSY) {
            printf("--> [ERROR] Cola de red llena. Transmisión saturada.\n");
        } else {
            printf("--> [ERROR] Falló al despachar trama a la red (Error: %d).\n", net_err);
        }
    } else {
        printf("--> [ERROR] Falló al empaquetar la trama (Error: %d).\n", frame_err);
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
            s_ready_announced = false;
            break;

        case CELL_STATE_STARTING:
            StatusIndicator_SetCellular(CELLULAR_STATUS_STARTING);
            s_ready_announced = false;
            break;

        case CELL_STATE_CONNECTING:
            StatusIndicator_SetCellular(CELLULAR_STATUS_SEARCHING);
            s_ready_announced = false;
            break;

        case CELL_STATE_READY:
            /* Solo actualizar a READY si no se está ejecutando una ráfaga de TRANSMITTING activa */
            if (StatusIndicator_GetCellular() != CELLULAR_STATUS_TRANSMITTING) {
                StatusIndicator_SetCellular(CELLULAR_STATUS_READY);
            }
            if (!s_ready_announced) {
                s_ready_announced = true;
                printf("\n==================================================\n");
                printf(" ¡RED CELULAR CONECTADA Y LISTA (CELL_STATE_READY)!\n");
                printf(" Presione el botón de pánico para emitir alertas.\n");
                printf("==================================================\n\n");
            }
            break;

        case CELL_STATE_ERROR:
            StatusIndicator_SetCellular(CELLULAR_STATUS_OFF);
            s_ready_announced = false;
            break;

        default:
            StatusIndicator_SetCellular(CELLULAR_STATUS_OFF);
            s_ready_announced = false;
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
        printf("[ERROR] Falló al inicializar StatusIndicator.\n");
    }

    /* 2. Inicialización del Handler del Botón de Pánico */
    panic_handler_err_t panic_err = PanicHandler_Init(OnPanicEvent);
    if (panic_err == PANIC_HANDLER_OK) {
        printf("[OK] PanicHandler inicializado con callback registrado.\n");
    } else {
        printf("[ERROR] Falló al inicializar PanicHandler (Error: %d).\n", panic_err);
    }

    /* 3. Instrucción de Energización y Cuenta Regresiva de Estabilización */
    printf("\n--------------------------------------------------\n");
    printf("[ACCION REQUERIDA] Energice la placa EVB (Switch VBAT en ON).\n");
    printf("[SISTEMA] Esperando estabilización de alimentación (%u segundos):\n", STABILIZATION_WAIT_SEC);
    for (int i = STABILIZATION_WAIT_SEC; i > 0; i--) {
        printf(" -> Estabilizando alimentación... %d s\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("[SISTEMA] Alimentación estabilizada. Inicializando servicio celular...\n");
    printf("--------------------------------------------------\n\n");

    /* 4. Inicialización del Servicio de Red Celular (FSM + Queue + Task) */
    cellular_net_err_t net_err = CellularNet_Init();
    if (net_err == CELL_NET_OK) {
        printf("[OK] CellularNet inicializado y tarea de red iniciada.\n");
    } else {
        printf("[ERROR] Falló al inicializar CellularNet (Error: %d).\n", net_err);
    }

    printf("\n[SISTEMA] ESP32 operativo. Estableciendo enlace con la red LTE...\n\n");

    /* 5. Bucle Principal de Ejecución No Bloqueante */
    for (;;) {
        /* Pasada de la FSM de detección de pulsación y debounce */
        PanicHandler_RunStep();

        /* Pasada del temporizador y parpadeo de LEDs de estado */
        StatusIndicator_RunStep();

        /* Actualización del patrón LED según el estado de la red celular */
        UpdateStatusLedFromNetwork();

        /* Gestión del temporizador de apagado del LED de pánico (auto-off tras 2s) */
        if (s_panic_led_active) {
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if ((now - s_panic_led_timer_ms) >= PANIC_LED_HOLD_TIME_MS) {
                StatusIndicator_SetPanic(false);
                s_panic_led_active = false;
            }
        }

        /* Liberar tiempo de CPU para la tarea de red y el sistema */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}