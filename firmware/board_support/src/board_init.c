#include "board_config.h"
#include "gpio_hal.h"
#include "uart_hal.h"
//#include "quectel_bsp.h"
#include "esp_log.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

static const char *TAG = "board_init";

void board_init(void)
{
    ESP_LOGI(TAG, "Board init start");

    /* Inicializaciones HAL genéricas */
    //GPIOInitDriver(); /* si tu gpio_hal define esto; si no, omitir */
    /* Inicializa UART con pines de board_config */
    UartHalInitWithPins(115200, UART1_TX_PIN, UART1_RX_PIN);

    /* Inicialización BSP específica del módem (usa solo HAL) */
    //Quectel_Init();

        /* Si necesitás power-on del módulo, hacelo aquí o desde quectel_bsp */
#ifdef QUECTEL_PWRKEY_PIN
    /* ejemplo no bloqueante de power toggle si hace falta:
       GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT, GPIO_PULL_NONE);
       GPIOOff(QUECTEL_PWRKEY_PIN);
    */
#endif

    ESP_LOGI(TAG, "Board init done");
}
