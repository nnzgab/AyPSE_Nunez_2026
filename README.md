<p align="center">
  <img src="banner.png" alt="Banner del Proyecto">
  <br>
</p>

### Arquitectura y Programación de Sistemas Embebidos 2026
#### **Autor:** Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)

##### Comunicador Celular Autónomo de Alerta de Pánico (ESP32-C6 + Quectel EG915U)
Sistema embebido de emisión remota de alertas de pánico desarrollado sobre el microcontrolador **ESP32-C6** (placa DevKitC-1) y el módem celular **Quectel EG915U-LA** (LTE Cat 1 bis). El dispositivo detecta la pulsación física de un botón de pánico mediante interrupción por flanco descendente con filtro antirebote no bloqueante de 50 ms, lee el IMEI real del hardware (`AT+CGSN`), empaqueta los datos de la alerta en formato texto CSV (`01,IMEI,SECUENCIA\r\n`) o binario estructurado con encabezado `0xAA55`, y transmite la información hacia un servidor TCP remoto a través de una tarea dedicada en FreeRTOS (`cell_net_task`) con cola de eventos (`xQueue`). Dispone de señalización visual mediante LEDs para monitorear los estados de la red celular y la activación de la alerta.
## Montaje del Hardware

A continuación se muestra cómo queda el protoboard con las placas armadas y listas para ejecutar el firmware:

<img src="https://github.com/nnzgab/AyPSE_Nunez_2026/blob/prueba_02_arregando_BSP/documentaci%C3%B3n/im%C3%A1genes/prototipo_small.jpg?raw=true" alt="Montaje del proyecto" width="50%">

[Video del sistema completo funcionando](https://www.youtube.com/watch?v=ejemplo_demostracion_tp4)

#### Diagrama general del sistema
```text
[ BOTÓN DE PÁNICO ] ──(GPIO23)──> [ ESP32-C6 MCU ] ──(UART1: 18/19)──> [ QUECTEL EG915U-LA ]
                                        │                                      │
[ LED PANIC (GPIO4) ] <─────────────────┤ (Control PWRKEY: GPIO6) ─────────────┤
[ LED QUECTEL (GPIO5) ] <──────────────┘                                      ▼
                                                                    [ RED CELULAR LTE Cat 1 ]
                                                                               │
[ SERVIDOR PYTHON / PINGGY (Puerto 8089) ] <──────────(Socket TCP/IP)──────────┘
```

#### Estructura del proyecto
```text
firmware/
├── apps/
│   └── 0_comunicador/
│       └── main/
│           └── 0_comunicador.c
├── middleware/
│   ├── inc/
│   │   ├── cellular_net.h
│   │   ├── event_frame.h
│   │   ├── panic_handler.h
│   │   └── status_indicator.h
│   ├── src/
│   │   ├── cellular_net.c
│   │   ├── event_frame.c
│   │   ├── panic_handler.c
│   │   └── status_indicator.c
│   └── test/
│       ├── test_cellular_net.c
│       ├── test_event_frame.c
│       ├── test_panic_handler.c
│       └── test_status_indicator.c
├── board_support/
│   ├── inc/
│   │   ├── board_clock.h
│   │   ├── board_config.h
│   │   ├── cellular_modem.h
│   │   ├── led.h
│   │   └── panic_button.h
│   ├── src/
│   │   ├── board_clock.c
│   │   ├── cellular_modem.c
│   │   ├── led.c
│   │   └── panic_button.c
│   └── test/
│       ├── test_cellular.c
│       ├── test_led.c
│       └── test_panic_button.c
├── drivers_hal/
│   ├── inc/
│   │   ├── gpio_hal.h
│   │   ├── gptimer_hal.h
│   │   └── uart_hal.h
│   ├── src/
│   │   ├── gpio_hal.c
│   │   ├── gptimer_hal.c
│   │   └── uart_hal.c
│   └── test/
│       ├── test_gpio_hal.c
│       ├── test_gptimer_hal.c
│       └── test_uart_hal.c
└── test_app/
    └── main/
        └── test_app_main.c

```

#### Módulos
##### apps (Aplicación)
* **`app_main()`**: Inicializa los módulos de estado (`StatusIndicator_Init`), el manejador del botón de pánico (`PanicHandler_Init`), ejecuta la cuenta regresiva de estabilización de energía e inicializa el servicio celular (`CellularNet_Init`). En su bucle principal no bloqueante atiende `PanicHandler_RunStep()`, `StatusIndicator_RunStep()`, `UpdateStatusLedFromNetwork()` y gestiona el temporizador de apagado del LED de pánico.
* **`OnPanicEvent()`**: Callback invocado al confirmarse la pulsación del botón; activa la indicación visual de pánico, obtiene el IMEI real, empaqueta la trama CSV y la deposita en la cola de transmisión de la red celular.

##### middleware (Middleware)
* **`panic_handler`**: Captura la interrupción física del botón de pánico (`GPIO23`), realiza la validación no bloqueante de debounce (50 ms) y gestiona la secuencia incremental de alertas.
* **`status_indicator`**: Controla y alterna de manera no bloqueante los patrones de parpadeo del LED indicador de módem (`GPIO5`) según el estado de la red (SEARCHING, READY, TRANSMITTING) y el estado del LED de pánico (`GPIO4`).
* **`event_frame`**: Encargado de la serialización de datos de eventos (`event_data_t`) en formato texto CSV (`01,IMEI,SECUENCIA\r\n`) or binario estructurado con encabezado `0xAA55`.
* **`cellular_net`**: Administra la Máquina de Estados Finitos (FSM) de conectividad celular LTE (STARTING, CONNECTING, READY, ERROR), gestiona el contexto PDP (`AT+QIACT`), lee el IMEI del hardware y procesa la cola FreeRTOS (`xQueue`) para transmitir tramas por socket TCP en segundo plano (`cell_net_task`).

##### board_support (BSP)
* **`cellular_modem`**: Implementa los comandos AT y el control por hardware del módem Quectel EG915U-LA vía UART1 (pulso PWRKEY en `GPIO6`, verificación de inicio, estado SIM/red, lectura IMEI y apertura/envío/cierre de sockets TCP).
* **`panic_button`**: Maneja el pulsador físico de pánico en `GPIO23`, configurando la interrupción por flanco descendente y filtrado inicial de hardware.
* **`led`**: Abstrae el encendido, apagado y conmutación de los LEDs de estado de pánico (`GPIO4`) y módem (`GPIO5`).
* **`board_clock`**: Provee servicios de marcas de tiempo e intervalos en milisegundos basados en el temporizador GPTimer de hardware.

##### drivers_hal (HAL)
* **`gpio_hal`**: Encapsula la configuración de dirección, nivel lógico e instalación de interrupciones sobre los pines GPIO del ESP32-C6.
* **`uart_hal`**: Abstrae la inicialización y comunicación serie bidireccional por UART1 (`GPIO18` TX / `GPIO19` RX) a 115200 baudios.
* **`gptimer_hal`**: Abstrae el temporizador GPTimer del ESP32-C6 configurado a 1 MHz (resolución de 1 µs) para la obtención de tiempos en milisegundos y retardos.

#### Imágenes
| | |
|:---:|:---:|
| ![Prototipo Físico Integrado](documentación/imágenes/prototipo_integrado.png) | ![Consola Serie / Logs](documentación/imágenes/log_consola_tcp.png) |
| *Placa ESP32-C6 conectada por UART1 al módem Quectel EG915U-LA y pulsador en protoboard* | *Log real por consola serie mostrando la conexión LTE y la transmisión de trama TCP* |
