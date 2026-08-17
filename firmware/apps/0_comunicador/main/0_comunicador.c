#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "uart_hal.h"
extern void StartBlinkLed1(void);

static const char *TAG = "0_comunicador";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting application (HAL)");
    UartHalInit(115200);
    ESP_LOGI(TAG, "UART HAL initialized");

    /* Arranca el blink en background sin bloquear */
    StartBlinkLed1();


    const char *msg = "Hello from HAL\n";
    char rx;

    while (1) {
        /* Envío periódico para verificar transmisión */
        UartHalWriteBytes(msg, strlen(msg));

        /* Intento de lectura: si llega algo, hago eco inmediato */
        int r = UartHalReadByte(&rx); // timeout 100 ms
        if (r > 0) {
            ESP_LOGI(TAG, "Received byte: 0x%02x '%c'", (uint8_t)rx, (rx >= 32 && rx < 127) ? rx : '.');
            UartHalWriteByte(rx); // eco
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
