<p align="center">
  <img src="banner.png" alt="Banner del Proyecto">
  <br>
</p>

### Arquitectura y Programación de Sistemas Embebidos 2026
#### **Autor:** Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)

# PROYECTO FINAL
## Comunicador Celular para Alertas de Eventos Pánico

## Idea General de la Aplicación
El presente proyecto consiste en el desarrollo de un comunicador celular para la transmisión de alertas ante eventos críticos, basado en un microcontrolador ESP32-C6 y un módem celular Quectel EG915U-LA (LTE Cat 1 bis). El sistema tiene como objetivo detectar un evento de pánico mediante un pulsador y transmitir una alerta hacia un servidor remoto utilizando la red celular, brindando una solución para situaciones sin conectividad cableada o Wi-Fi. 

Como caso de uso, el sistema puede aplicarse a dispositivos de alerta personal, instalaciones residenciales o sistemas de monitoreo de infraestructura. La alerta incorpora la identificación del equipo mediante el IMEI real del módem celular y se transmite mediante una conexión TCP, esperando la confirmación del servidor.

### Especificaciones Técnicas Principales
* **Microcontrolador:** ESP32-C6 (DevKitC-1) utilizando FreeRTOS y arquitectura por capas.
* **Módem Celular:** Quectel EG915U-LA (LTE Cat 1 bis) integrado mediante comandos AT por UART.
* **Manejo de Eventos:** Interrupción por flanco descendente en el botón de pánico con filtro antirebote no bloqueante (50 ms) y encolado de eventos con `xQueue`.
* **Protocolo de Comunicación:** Transmisión de tramas (CSV o binarias con encabezado `0xAA55`) hacia un servidor remoto mediante sockets TCP en una tarea dedicada (`cell_net_task`).
* **Interfaz Visual:** Indicación por LEDs de estado para monitoreo de la red celular y confirmación de alerta.

---

## Montaje del Hardware

El prototipo físico fue ensamblado y probado sobre el banco de trabajo utilizando los siguientes componentes integrados:
* **Microcontrolador:** Espressif ESP32-C6 operando con el framework ESP-IDF v6.0.1.
* **Módem Celular:** Quectel EG915U-LA montado sobre la placa de evaluación Quectel UMTS&LTE EVB, alimentado mediante la línea VBAT.
* **Interfaz UART:** Bus serie UART1 configurado a 115200 baudios (TX en GPIO18, RX en GPIO19).
* **Línea PWRKEY:** Transistor de control conectado a GPIO6 para la secuencia de encendido por pulso.
* **Entrada de Alerta:** Pulsador de pánico con resistencia pull-up e interrupción activada por flanco descendente.
* **Señalización Visual:** LEDs indicadores de estado conectados a GPIO4 (alerta de pánico) y GPIO5 (monitoreo de red celular y transmisión).

A continuación se muestra el ensamblaje en el protoboard:

<p align="center">
  <img src="https://github.com/nnzgab/AyPSE_Nunez_2026/blob/prueba_02_arregando_BSP/documentaci%C3%B3n/im%C3%A1genes/prototipo_small.jpg?raw=true" alt="Montaje del proyecto" width="50%">
</p>

---

## Demostración en Video

Para respaldar la validación funcional en hardware real, se grabó un video demostrativo que exhibe la operación completa del sistema en tiempo real. 

**La secuencia demostrada incluye:**
* Secuencia de inicialización y pulso PWRKEY para encendido del módem.
* Proceso de registro en la red celular LTE y obtención de la dirección IP por PDP.
* Presión del pulsador de pánico y disparo inmediato de la interrupción por hardware.
* Parpadeo rápido del LED indicador durante la ráfaga de transmisión del socket TCP.
* Impresión en la consola de la trama enviada con el IMEI real y la recepción de la respuesta ACK del servidor remoto.

**Hacé clic en la imagen a continuación para ver el video:**

<p align="center">
  <a href="https://www.youtube.com/shorts/6NaJSEQukmM" target="_blank">
    <img src="https://img.youtube.com/vi/6NaJSEQukmM/0.jpg" alt="Video demostrativo del sistema" width="300">
  </a>
</p>

---

## Arquitectura del Sistema

La implementación se organiza mediante una arquitectura por capas estricta: <br>
**Aplicación → Middleware → Board Support → Driver HAL → ESP-IDF**. <br>
Esta separación permite mantener aislada la lógica de la aplicación respecto de los detalles específicos del hardware.

<p align="center">
  <img src="https://github.com/nnzgab/AyPSE_Nunez_2026/blob/prueba_02_arregando_BSP/documentaci%C3%B3n/im%C3%A1genes/diagrama_bloque.png?raw=true" alt="Diagrama general del sistema" width="700">
</p>

### Estructura del proyecto
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

### Descripción de Módulos

#### apps (Aplicación)
* **`app_main()`**: Inicializa los módulos de estado (`StatusIndicator_Init`), el manejador del botón de pánico (`PanicHandler_Init`), ejecuta la cuenta regresiva de estabilización de energía e inicializa el servicio celular (`CellularNet_Init`). En su bucle principal no bloqueante atiende `PanicHandler_RunStep()`, `StatusIndicator_RunStep()`, `UpdateStatusLedFromNetwork()` y gestiona el temporizador de apagado del LED de pánico.
* **`OnPanicEvent()`**: Callback invocado al confirmarse la pulsación del botón; activa la indicación visual de pánico, obtiene el IMEI real, empaqueta la trama CSV y la deposita en la cola de transmisión de la red celular.

#### middleware (Middleware)
* **`panic_handler`**: Captura la interrupción física del botón de pánico (`GPIO23`), realiza la validación no bloqueante de debounce (50 ms) y gestiona la secuencia incremental de alertas.
* **`status_indicator`**: Controla y alterna de manera no bloqueante los patrones de parpadeo del LED indicador de módem (`GPIO5`) según el estado de la red (SEARCHING, READY, TRANSMITTING) y el estado del LED de pánico (`GPIO4`).
* **`event_frame`**: Encargado de la serialización de datos de eventos (`event_data_t`) en formato texto CSV (`01,IMEI,SECUENCIA\r\n`) o binario estructurado con encabezado `0xAA55`.
* **`cellular_net`**: Administra la Máquina de Estados Finitos (FSM) de conectividad celular LTE (STARTING, CONNECTING, READY, ERROR), gestiona el contexto PDP (`AT+QIACT`), lee el IMEI del hardware y procesa la cola FreeRTOS (`xQueue`) para transmitir tramas por socket TCP en segundo plano (`cell_net_task`).

#### board_support (BSP)
* **`cellular_modem`**: Implementa los comandos AT y el control por hardware del módem Quectel EG915U-LA vía UART1 (pulso PWRKEY en `GPIO6`, verificación de inicio, estado SIM/red, lectura IMEI y apertura/envío/cierre de sockets TCP).
* **`panic_button`**: Maneja el pulsador físico de pánico en `GPIO23`, configurando la interrupción por flanco descendente y filtrado inicial de hardware.
* **`led`**: Abstrae el encendido, apagado y conmutación de los LEDs de estado de pánico (`GPIO4`) y módem (`GPIO5`).
* **`board_clock`**: Provee servicios de marcas de tiempo e intervalos en milisegundos basados en el temporizador GPTimer de hardware.

#### drivers_hal (HAL)
* **`gpio_hal`**: Encapsula la configuración de dirección, nivel lógico e instalación de interrupciones sobre los pines GPIO del ESP32-C6.
* **`uart_hal`**: Abstrae la inicialización y comunicación serie bidireccional por UART1 (`GPIO18` TX / `GPIO19` RX) a 115200 baudios.
* **`gptimer_hal`**: Abstrae el temporizador GPTimer del ESP32-C6 configurado a 1 MHz (resolución de 1 µs) para la obtención de tiempos en milisegundos y retardos.

---

### Consola de Depuración

<p align="center">
  <img src="https://github.com/nnzgab/AyPSE_Nunez_2026/blob/main/documentaci%C3%B3n/im%C3%A1genes/Captura%20desde%202026-09-21%2023-21-56.png?raw=true" alt="Consola Serie / Logs" width="70%">
  <br>|
  <em>Log real por consola serie mostrando la conexión LTE y la transmisión de trama TCP</em>

