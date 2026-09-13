/*

APP
│
│  FreeRTOS
│  tareas
│  delays
│  sincronización
│  planificación
▼
MIDDLEWARE
│
│  lógica de servicios
│  FSM
│  procesamiento
│  interfaces
▼
BSP
│
│  comportamiento específico del módulo/placa
▼
HAL
│
│  abstracción de periféricos
▼
ESP-IDF


├── apps
│   ├── 0_comunicador
│   │   ├── CMakeLists.txt
│   │   ├── Doxyfile
│   │   └── main
│   │        └── 0_comunicador.c
├── board_support
│   ├── inc
│   │   ├── board_config.h
│   │   ├── cellular.h
│   │   ├── cellular_modem.h
│   │   ├── led.h
│   │   ├── panic_button.h
│   │   └── template_bsp.h
│   ├── src
│   │   ├── cellular.c
│   │   ├── cellular_modem.c
│   │   ├── led.c
│   │   ├── panic_button.c
│   │   └── template_bsp.c
│   └── test
│       ├── test_cellular.c
│       ├── test_led.c
│       └── test_panic_button.c
├── drivers_hal
│   ├── CMakeLists.txt
│   ├── inc
│   │   ├── gpio_hal.h
│   │   ├── template_hal.h
│   │   └── uart_hal.h
│   ├── src
│   │   ├── gpio_hal.c
│   │   ├── template_hal.c
│   │   └── uart_hal.c
│   └── test
│       ├── test_gpio_hal.c
│       └── test_uart_hal.c
├── middleware
│   ├── CMakeLists.txt
│   ├── inc
│   │   ├── cellular_net.h
│   │   ├── event_frame.h
│   │   ├── panic_handler.h
│   │   └── status_indicator.h
│   ├── src
│   │   ├── cellular_net.c
│   │   ├── event_frame.c
│   │   ├── panic_handler.c
│   │   └── status_indicator.c
│   └── test
│       ├── test_cellular_net.c
│       ├── test_panic_handler.c
│       └── test_status_indicator.c
└── test_app
    ├── CMakeLists.txt
    ├── main
    │   ├── CMakeLists.txt
    │   └── test_app_main.c
    └── README.md
 */


 #include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Capa BSP (Board Support Package) */
#include "led.h"
#include "panic_button.h"
#include "cellular_modem.h"

/* Capa Middleware */
#include "status_indicator.h"
#include "cellular_net.h"
#include "panic_handler.h"

#define TAG "APP_MAIN"

#define CELL_NET_TASK_STACK_SIZE    4096
#define CELL_NET_TASK_PRIORITY      3

#define PANIC_TASK_STACK_SIZE       2048
#define PANIC_TASK_PRIORITY         5  /* Mayor prioridad para respuesta inmediata */

/**
 * @brief Lee el IMEI del módem celular durante la secuencia de arranque.
 * 
 * @param[out] imei_out Buffer donde se almacenará el IMEI (mínimo 16 bytes).
 * @param[in] max_len Tamaño máximo del buffer.
 * @return true si se leyó correctamente el IMEI, false si falló.
 */
static bool fetch_device_imei(char *imei_out, size_t max_len) {
    uint8_t rx_buf[64] = {0};
    
    // Envía el comando AT estándar 3GPP para consultar el IMEI
    const char *cmd_imei = "AT+CGSN\r\n";
    CellularModemWriteRaw((const uint8_t *)cmd_imei, (uint16_t)strlen(cmd_imei));

    // Espera la respuesta por la UART (timeout 2000 ms)
    uint16_t read_bytes = CellularModemReadRaw(rx_buf, sizeof(rx_buf) - 1, 2000);
    if (read_bytes == 0) {
        return false;
    }
    
    rx_buf[read_bytes] = '\0';

    // Parsea la respuesta buscando los 15 dígitos continuos del IMEI
    char *ptr = (char *)rx_buf;
    while (*ptr != '\0') {
        if (*ptr >= '0' && *ptr <= '9') {
            size_t len = 0;
            while (ptr[len] >= '0' && ptr[len] <= '9' && len < 15) {
                len++;
            }
            if (len == 15 && (max_len > 15)) {
                strncpy(imei_out, ptr, 15);
                imei_out[15] = '\0';
                return true;
            }
        }
        ptr++;
    }

    return false;
}

void app_main(void) {
    char device_imei[16] = {0};

    /* -------------------------------------------------------------------- */
    /* 1. Inicialización de Hardware (BSP)                                  */
    /* -------------------------------------------------------------------- */
    if (!LedInit()) {
        printf("[%s] Error al inicializar LEDs del BSP\n", TAG);
        return;
    }

    if (!PanicButtonInit()) {
        printf("[%s] Error al inicializar Botón de Pánico\n", TAG);
        return;
    }

    if (!CellularModemInit()) {
        printf("[%s] Error al inicializar UART/Control del Módem\n", TAG);
        return;
    }

    /* -------------------------------------------------------------------- */
    /* 2. Inicialización del Middleware                                     */
    /* -------------------------------------------------------------------- */
    if (!StatusIndicator_Init()) {
        printf("[%s] Error al inicializar Indicador de Estado\n", TAG);
        return;
    }

    if (CellularNet_Init() != CELL_NET_OK) {
        printf("[%s] Error al inicializar Cola de Red Celular\n", TAG);
        return;
    }

    /* Establece estado inicial del LED: Arrancando Módem */
    StatusIndicator_SetCellular(CELLULAR_STATUS_STARTING);

    /* Enciende físicamente el módem */
    CellularModemPowerPulse();

    /* -------------------------------------------------------------------- */
    /* 3. Captura Única de IMEI (Fase de Arranque)                         */
    /* -------------------------------------------------------------------- */
    StatusIndicator_SetCellular(CELLULAR_STATUS_SEARCHING);
    
    uint8_t imei_retries = 0;
    while (!fetch_device_imei(device_imei, sizeof(device_imei))) {
        imei_retries++;
        if (imei_retries >= 5) {
            // Si no responde tras 5 reintentos, asigna un IMEI fallback de prueba
            strncpy(device_imei, "000000000000000", 15);
            device_imei[15] = '\0';
            printf("[%s] Advertencia: No se obtuvo IMEI. Asignado IMEI por defecto: %s\n", TAG, device_imei);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("[%s] Sistema Iniciado. IMEI Dispositivo: %s\n", TAG, device_imei);

    /* Inicializa el manejador de pánico con el IMEI cacheado */
    if (PanicHandler_Init(device_imei) != PANIC_HANDLER_OK) {
        printf("[%s] Error al inicializar Panic Handler\n", TAG);
        return;
    }

    /* -------------------------------------------------------------------- */
    /* 4. Vinculación de Interrupción de Hardware (BSP -> Middleware)      */
    /* -------------------------------------------------------------------- */
    PanicButtonAttachInterrupt((panic_button_isr_cb_t)PanicHandler_OnButtonISR, NULL);
    /* -------------------------------------------------------------------- */
    /* 5. Creación de Tareas de FreeRTOS                                    */
    /* -------------------------------------------------------------------- */
    xTaskCreate(CellularNet_Task,
                "CellularNet_Task",
                CELL_NET_TASK_STACK_SIZE,
                NULL,
                CELL_NET_TASK_PRIORITY,
                NULL);

    xTaskCreate(PanicHandler_Task,
                "PanicHandler_Task",
                PANIC_TASK_STACK_SIZE,
                NULL,
                PANIC_TASK_PRIORITY,
                NULL);

    /* -------------------------------------------------------------------- */
    /* 6. Fin de app_main (Las tareas y el scheduler continúan)            */
    /* -------------------------------------------------------------------- */
    printf("[%s] Tareas creadas. Sistema operando en modo asíncrono.\n", TAG);
}