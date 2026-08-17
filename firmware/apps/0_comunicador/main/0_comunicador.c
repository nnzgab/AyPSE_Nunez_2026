#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* Inclusiones correctas: Solo BSP y Middleware (Cero HAL en la App) */
#include "led.h"
#include "panic_button.h"
#include "cellular.h"
#include "event_frame.h"

static const char *TAG = "cellular_gateway";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Cellular Gateway Application...");

    /* 1. Inicialización de periféricos a través de la capa BSP */
    LedInit();
    PanicButtonInit();
    
    if (CellularInit()) {
        ESP_LOGI(TAG, "Cellular module initialized successfully");
    } else {
        ESP_LOGE(TAG, "Failed to initialize cellular module");
    }

    /* Configurar PDP del operador celular */
    if (CellularPdpConfigure("internet.gprs.com")) {
        CellularPdpActivate();
    }

    uint8_t tx_buffer[256];
    size_t buffer_len;

    while (1) {
        /* 2. Ejemplo de lógica usando el botón de pánico de la BSP */
        if (PanicButtonIsPressed()) {
            ESP_LOGW(TAG, "Panic button pressed! Dispatching event frame...");
            
            /* Encender LED de pánico como feedback visual */
            LedOn(LED_PANIC);

            /* Construir la trama a través del Middleware */
            buffer_len = sizeof(tx_buffer);
            if (EventFrameBuild(1, "TOKEN_XYZ123", tx_buffer, &buffer_len)) {
                
                /* Enviar mediante el flujo de TCP con reintentos y fallback a SMS */
                if (EventFrameDispatch(tx_buffer, buffer_len)) {
                    ESP_LOGI(TAG, "Event dispatched successfully (TCP or SMS)");
                } else {
                    ESP_LOGE(TAG, "Critical: Failed to dispatch event via TCP and SMS");
                }
            }

            LedOff(LED_PANIC);
        }

        /* Parpadeo periódico del LED del Quectel para indicar que el sistema está vivo */
        LedToggle(LED_QUECTEL);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}