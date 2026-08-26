#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

/* BSP */
#include "led.h"
#include "panic_button.h"
#include "cellular.h"

/* Middleware */
#include "event_frame.h"

static const char *TAG = "cellular_gateway";
void app_main(void)
{
    ESP_LOGI(TAG, "=== TEST UART EG915U ===");

    UartHalInit(UART_BAUDRATE);

    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1)
    {
        char response[256];

        ESP_LOGI(TAG, "Enviando AT...");

        UartHalWriteBytes(
            "AT\r\n",
            4
        );

        int len = UartHalReadBytes(
            response,
            sizeof(response) - 1,
            3000
        );

        if (len > 0)
        {
            response[len] = '\0';

            ESP_LOGI(
                TAG,
                "RX (%d bytes):\n%s",
                len,
                response
            );
        }
        else
        {
            ESP_LOGW(
                TAG,
                "No se recibieron datos"
            );
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}