#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led.h"
#include "cellular.h"

static const char *TAG = "test_uart_bsp";

void app_main(void)
{
    ESP_LOGI(TAG, "--- Prueba de UART desde la capa BSP ---");

    LedInit();

    /* 1. Inicializar la UART interna llamando a la BSP */
    ESP_LOGI(TAG, "Inicializando la UART del módem a través de CellularInit()...");
    
    if (CellularInit()) {
        ESP_LOGI(TAG, "BSP: UART1 configurada correctamente.");
    } else {
        ESP_LOGE(TAG, "BSP: Falló la inicialización de la UART.");
    }

    while (1) {
        /* Heartbeat */
        LedToggle(LED_QUECTEL);

        /* 2. Enviar comando de prueba a través de la BSP */
        ESP_LOGI(TAG, "Enviando comando AT mediante la BSP...");
        
        /* Con el puente TX<->RX hecho, la UART transmitirá por GPIO18 y 
           recibirá el eco exacto por GPIO19 sin bloquear la arquitectura */
        if (CellularPdpConfigure("internet.gprs.com")) {
            ESP_LOGW(TAG, ">>> Respuesta recibida en la BSP <<<");
            LedOn(LED_PANIC);
            vTaskDelay(pdMS_TO_TICKS(100));
            LedOff(LED_PANIC);
        } else {
            ESP_LOGE(TAG, "Sin respuesta / Timeout en la BSP.");
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}