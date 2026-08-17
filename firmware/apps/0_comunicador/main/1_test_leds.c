#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led.h"

static const char *TAG = "test_leds";

/* Tarea 1: Parpadeo Rápido (LED Quectel) */
static void vTaskLedQuectel(void *pvParameters)
{
    while (1) {
        LedToggle(LED_QUECTEL);
        vTaskDelay(pdMS_TO_TICKS(150)); /* Conmuta cada 150 ms */
    }
}

/* Tarea 2: Parpadeo Lento (LED Pánico) */
static void vTaskLedPanic(void *pvParameters)
{
    while (1) {
        LedToggle(LED_PANIC);
        vTaskDelay(pdMS_TO_TICKS(1000)); /* Conmuta cada 1000 ms */
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "--- Iniciando prueba de LEDs (BSP) ---");

    /* 1. Inicializar periféricos desde la BSP */
    LedInit();

    /* 2. Crear dos tareas independientes de FreeRTOS */
    xTaskCreate(vTaskLedQuectel, "task_quectel", 2048, NULL, 5, NULL);
    xTaskCreate(vTaskLedPanic,   "task_panic",   2048, NULL, 5, NULL);
}