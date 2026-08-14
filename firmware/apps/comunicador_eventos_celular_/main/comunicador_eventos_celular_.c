#include "uart_hal.h"
#include <string.h>

void app_main(void)
{
    uart_hal_cfg_t cfg = {
        .port = 1,
        .tx_pin = 18,
        .rx_pin = 19,
        .baudrate = 115200,
        .rx_buffer_size = 1024,
        .tx_buffer_size = 1024,
        .use_flow_ctrl = false
    };

    if (!UartHalInit(&cfg)) {
        printf("Uart init failed\n");
        return;
    }

    const char *cmd = "AT\r\n";
    UartHalWrite(cfg.port, cmd, strlen(cmd), pdMS_TO_TICKS(1000));

    char buf[256];
    int r = UartHalRead(cfg.port, buf, sizeof(buf)-1, pdMS_TO_TICKS(2000));
    if (r > 0) {
        buf[r] = '\0';
        printf("RX: %s\n", buf);
    }

    /* dejar driver instalado durante la vida de la app o llamar UartHalDeinit al final */
