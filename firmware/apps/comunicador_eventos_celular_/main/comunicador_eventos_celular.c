/*! @mainpage Comunicador Eventos Celular
 *
 * @section genDesc General Description
 *
 * Programa de prueba mínimo para compilar el proyecto y verificar el HAL UART.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32    |
 * |:--------------:|:-----------:|
 * |  UART TX       |  GPIO_X     |
 * |  UART RX       |  GPIO_Y     |
 *
 * @section changelog Changelog
 *
 * |   Date      | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 13/08/2026 | Creación del main de prueba                     |
 *
 * @author Gabriel
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_hal.h"
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void)
{
    printf("Hello world!\n");

    /* Ejemplo opcional: inicializar el HAL UART para comprobar linkeo.
     * Descomentá si querés probar la inicialización del driver.
     *
     * Ajustá uart_num y pines según tu BSP.
     */
#if 0
    uart_hal_cfg_t cfg = {
        .uart_num = 1,
        .tx_pin = 17,
        .rx_pin = 16,
        .baudrate = 115200,
        .rx_buffer_size = 256,
        .tx_buffer_size = 256
    };

    if (UartHalInit(&cfg)) {
        printf("UartHalInit OK\n");
    } else {
        printf("UartHalInit FAILED\n");
    }
#endif

    /* Mantener la aplicación viva */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
/*==================[end of file]============================================*/
