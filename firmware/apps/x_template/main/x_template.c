/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Hello world!\n");
}
/*==================[end of file]============================================*/


/*
void app_main(void)
{
    // Inicialización 

    ButtonPanicInit();

    StatusIndicatorInit();

    CellularNetInit();

    // Inicialización del módem/red

    CellularNetConfigurePdp(...);

    CellularNetActivate();

    //  Esperar conexión 

    while (1)
    {
        StatusIndicatorRunStep();

        if (ButtonPanicIsPressed())
        {
            // Evento de pánico 

            ...
        }

        vTaskDelay(...);
    }
}
*/

/*
typedef enum
{
    APP_STATE_INIT,
    APP_STATE_WAIT_MODEM,
    APP_STATE_CONNECT,
    APP_STATE_IDLE,
    APP_STATE_SEND_PANIC,
    APP_STATE_WAIT_ACK
} app_state_t;
*/

/*

APP → decide qué debe hacer el sistema.
Middleware → implementa servicios reutilizables.
BSP → conoce el hardware de la placa/módulo.
HAL → conoce los periféricos básicos.

INIT
 │
 ▼
WAIT_MODEM
 │
 ▼
CONNECT
 │
 ▼
IDLE
 │
 │ botón
 ▼
SEND_PANIC
 │
 ▼
WAIT_ACK
 │
 ▼
IDLE
*/

/*
La aplicación podría llegar a hacer:

EventFrameCreate(&event, EVENT_PANIC, imei);
CellularNetSendEvent(&event);

Y EventFrame se encarga de la representación.

Eso es mucho mejor que terminar con algo como:

sprintf(buffer,
        "PANIC|%s|%s|%d",
        imei,
        timestamp,
        crc);

metido dentro de app_main()
*/

/*
Capa	Componente	Responsabilidad
APP	app_main	Lógica general y estados del sistema
Middleware	event_frame	Construcción de eventos/tramas
Middleware	cellular_net	Servicio de comunicación TCP
Middleware	status_indicator	Lógica de estados de LEDs
BSP	button_panic	Manejo del pulsador
BSP	led	Control de LEDs
BSP	cellular_modem	EG915U + comandos AT + TCP
HAL	gpio_hal	GPIO
HAL	uart_hal	UART
*/


/*
apps/
└── 0_comunicador/
    └── app_main.c

middleware/
├── inc/
│   ├── event_frame.h
│   └── cellular_net.h
└── src/
    ├── event_frame.c
    └── cellular_net.c

board_support/
├── inc/
│   ├── button_panic.h
│   ├── led.h
│   └── cellular_modem.h
└── src/
    ├── button_panic.c
    ├── led.c
    └── cellular_modem.c

drivers_hal/
├── inc/
│   ├── gpio_hal.h
│   └── uart_hal.h
└── src/
    ├── gpio_hal.c
    └── uart_hal.c
*/

/*
CellularModem transporta bytes.
EventFrame construye el mensaje.
CellularNet ofrece comunicación.
APP decide qué hacer cuando recibe ACK.
*/