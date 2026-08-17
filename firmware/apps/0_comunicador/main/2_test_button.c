#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led.h"
#include "panic_button.h"

static const char *TAG = "test_button";

void app_main(void)
{
    ESP_LOGI(TAG, "--- Iniciando prueba de Entrada Digital (BSP) ---");

    /* 1. Inicialización de periféricos a través de la BSP */
    LedInit();
    PanicButtonInit();

    while (1) {
        /* 2. Evaluar el estado del botón/cable simulador */
        if (PanicButtonIsPressed()) {
            ESP_LOGW(TAG, "¡Botón de pánico PRESIONADO!");
            LedOn(LED_PANIC);
        } else {
            LedOff(LED_PANIC);
        }

        /* Heartbeat rápido (200 ms) para verificar que el bucle no se bloquea */
        LedToggle(LED_QUECTEL);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}