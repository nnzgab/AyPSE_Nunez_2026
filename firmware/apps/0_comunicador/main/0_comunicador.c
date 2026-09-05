/*! @mainpage Comunicador de Pánico Celular
 *
 * \section genDesc General Description
 *
 * Aplicación principal basada en una máquina de estados no bloqueante que coordina
 * la lectura de un botón de pánico, indicadores visuales por LED y la transmisión
 * de eventos vía módem celular por TCP.
 *
 * @section changelog Changelog
 *
 * |   Date     | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 04/09/2026 | Document creation                              |
 *
 * @author Gabriel Eduardo Núñez (nunezgabrieleduardo@gmail.com) *
 * 
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"

#include "uart_hal.h"
#include "panic_handler.h"
#include "status_indicator.h"
#include "cellular_net.h"
#include "cellular_modem.h"
#include "event_frame.h"

/*==================[macros and definitions]=================================*/
#define UART_BAUDRATE    115200
#define CONFIG_STEP_PERIOD 20   /* Período del loop principal en ms */
#define APP_APN          "datos.personal.com"
#define APP_SERVER_IP    "hkmwp-190-183-23-94.run.pinggy-free.link"
#define APP_SERVER_PORT  33027
#define APP_ACK_STR      "ACK"

/*==================[internal data definition]===============================*/
typedef enum {
    APP_STATE_STARTING = 0,
    APP_STATE_REGISTERING,
    APP_STATE_CONNECTING_APN,
    APP_STATE_READY,
    APP_STATE_OPENING_TCP,
    APP_STATE_SENDING,
    APP_STATE_WAITING_ACK,
    APP_STATE_CLOSING
} app_state_t;

static app_state_t s_state = APP_STATE_STARTING;
static char s_imei[32] = {0};

/*==================[internal functions declaration]=========================*/
static void UpdateCellularLed(void);
static void ComunicadorInit(void);
static void ComunicadorRunStep(void);

/*==================[internal functions definition]==========================*/
static void UpdateCellularLed(void)
{
    switch (s_state)
    {
        case APP_STATE_STARTING:
            StatusIndicatorSetCellular(CELLULAR_STATUS_STARTING);
            break;
        case APP_STATE_REGISTERING:
            StatusIndicatorSetCellular(CELLULAR_STATUS_SEARCHING);
            break;
        case APP_STATE_READY:
            StatusIndicatorSetCellular(CELLULAR_STATUS_READY);
            break;
        case APP_STATE_OPENING_TCP:
        case APP_STATE_SENDING:
        case APP_STATE_WAITING_ACK:
        case APP_STATE_CLOSING:
            StatusIndicatorSetCellular(CELLULAR_STATUS_TRANSMITTING);
            break;
        default:
            break;
    }
}

static void ComunicadorInit(void)
{
    UartHalInit(UART_BAUDRATE);

    if (!PanicHandlerInit()) {
        printf("Error crítico: Falló init de PanicHandler\n");
    }
    if (!StatusIndicatorInit()) {
        printf("Error crítico: Falló init de StatusIndicator\n");
    }

    StatusIndicatorSetCellular(CELLULAR_STATUS_STARTING);
    StatusIndicatorRunStep();

    if (CellularNetInit() != ESP_OK) {
        printf("Error crítico: Falló init de CellularNet\n");
    }

    s_state = APP_STATE_REGISTERING;
}

static void ComunicadorRunStep(void)
{
    /* 1. Actualizar capas de bajo nivel de forma continua y no bloqueante */
    PanicHandlerRunStep();
    StatusIndicatorSetPanic(PanicHandlerIsActive());

    /* 2. Máquina de estados de la aplicación */
    switch (s_state)
    {
        case APP_STATE_REGISTERING:
            if (CellularNetGetStatus() != NET_STATUS_DISCONNECTED)
            {
                CellularModemGetImei(s_imei, sizeof(s_imei));
                printf("Módem registrado. IMEI obtenido: %s\n", s_imei);
                s_state = APP_STATE_CONNECTING_APN;
            }
            break;

        case APP_STATE_CONNECTING_APN:
            if (CellularNetConnectApn(APP_APN, "", "") == ESP_OK)
            {
                printf("Contexto PDP activado correctamente.\n");
                s_state = APP_STATE_READY;
            }
            break;

        case APP_STATE_READY:
            if (PanicHandlerIsActive())
            {
                printf("¡Botón de pánico presionado! Iniciando transmisión...\n");
                s_state = APP_STATE_OPENING_TCP;
            }
            break;

        case APP_STATE_OPENING_TCP:
            if (CellularNetOpenTcp(APP_SERVER_IP, APP_SERVER_PORT) == ESP_OK)
            {
                s_state = APP_STATE_SENDING;
            }
            break;

        case APP_STATE_SENDING:
        {
            event_frame_t evt = { .type = EVENT_TYPE_PANIC };
            
            size_t imei_len = strlen(s_imei);
            if (imei_len >= sizeof(evt.imei)) {
                imei_len = sizeof(evt.imei) - 1;
            }
            memcpy(evt.imei, s_imei, imei_len);
            evt.imei[imei_len] = '\0';

            char frame[64];
            int len = EventFrameBuild(&evt, frame, sizeof(frame));

            if (len > 0 && CellularNetSendData((const uint8_t *)frame, (size_t)len) == ESP_OK)
            {
                s_state = APP_STATE_WAITING_ACK;
            }
            break;
        }

        case APP_STATE_WAITING_ACK:
        {
            uint8_t rx[32] = {0};
            size_t received = 0;

            if (CellularNetReceiveData(rx, sizeof(rx) - 1, &received) == ESP_OK && received > 0)
            {
                rx[received] = '\0';
                printf("Respuesta recibida del servidor: %s\n", rx);

                if (strcmp((const char *)rx, APP_ACK_STR) == 0)
                {
                    printf("¡ACK confirmado! Reseteando alarma.\n");
                    PanicHandlerClear();
                }
                s_state = APP_STATE_CLOSING;
            }
            break;
        }

        case APP_STATE_CLOSING:
            CellularNetCloseTcp();
            s_state = APP_STATE_READY;
            printf("Sistema listo para nuevas alertas.\n");
            break;

        default:
            break;
    }

    /* 3. Sincronizar indicadores visuales y ejecutar su tick */
    UpdateCellularLed();
    StatusIndicatorRunStep();
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    printf("=== INICIANDO APLICACION COMUNICADOR DE PANICO ===\n");

    ComunicadorInit();

    while (true)
    {
        ComunicadorRunStep();
        vTaskDelay(pdMS_TO_TICKS(CONFIG_STEP_PERIOD));
    }
}
/*==================[end of file]============================================*/