#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led.h"
#include "blink_task.h"

static const char *TAG = "blink_task";
static TaskHandle_t blink_task_handle = NULL;
static uint32_t blink_period_ms = 500;

static void blink_led1_task(void *arg)
{
    (void)arg;
    LedsInit();            /* asegura inicialización */
    for (;;) {
        LedToggle(LED_1);
        vTaskDelay((pdMS_TO_TICKS(blink_period_ms)));
    }
}

/* Función pública para arrancar la tarea */
void StartBlinkLed1(void)
{
    if (blink_task_handle == NULL) {
        xTaskCreate(blink_led1_task, "blink_led1", 2048, NULL, tskIDLE_PRIORITY + 1, &blink_task_handle);
        ESP_LOGI(TAG, "Blink task started");
    }
}

void StopBlinkLed1(void)
{
    if (blink_task_handle != NULL) {
        vTaskDelete(blink_task_handle);
        blink_task_handle = NULL;
        ESP_LOGI(TAG, "Blink task stopped");
    }
}
