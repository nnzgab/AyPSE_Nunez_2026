#include "quectel_bsp.h"
#include "board_config.h"
#include "uart_hal.h"
#include "gpio_hal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "quectel_bsp";

uint8_t Quectel_Init(void)
{
    /* 1. Configurar recursos via HAL */
    /* Inicializa UART con pines definidos en board_config.h */
    UartHalInitWithPins(115200, UART1_TX_PIN, UART1_RX_PIN);

    /* 2. Configurar pines del módulo (PWRKEY, RESET) */
#ifdef QUECTEL_PWRKEY_PIN
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT, GPIO_PULL_NONE);
    GPIOOff(QUECTEL_PWRKEY_PIN);
#endif

    /* 3. Estado conocido y retorno */
    ESP_LOGI(TAG, "Quectel_Init done");
    return 1;
}

uint8_t Quectel_PowerOn(void)
{
#ifdef QUECTEL_PWRKEY_PIN
    GPIOOn(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(200));
    GPIOOff(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(2000));
    return 1;
#else
    return 0;
#endif
}

uint8_t Quectel_PowerOff(void)
{
#ifdef QUECTEL_PWRKEY_PIN
    GPIOOn(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(200));
    GPIOOff(QUECTEL_PWRKEY_PIN);
    return 1;
#else
    return 0;
#endif
}

int Quectel_SendRaw(const char *buf, size_t len)
{
    if (!buf || len == 0) return 0;
    UartHalWriteBytes(buf, len);
    return (int)len;
}

int Quectel_ReadRaw(char *buf, size_t maxlen, uint32_t timeout_ms)
{
    return UartHalReadBytes(buf, maxlen, timeout_ms);
}
