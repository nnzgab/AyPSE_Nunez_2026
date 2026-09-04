#include "unity.h"
#include "esp_err.h"

#include "uart_hal.h"
#define UART_BAUDRATE 115200

#include "cellular_net.h"

#include <string.h>
#include <stdint.h>

TEST_CASE("tests del middleware", "[cellular_net]")
{
    uint8_t frame[] = "PANIC,869671077009056";
    uint8_t rx_buffer[128];

    size_t received = 0;

    /* ========================================================
     * 1. Inicializar red celular
     * ======================================================== */

    //TEST_ASSERT_EQUAL(ESP_OK, CellularNetInit());
    UartHalInit(UART_BAUDRATE);

    TEST_ASSERT_EQUAL(NET_STATUS_DISCONNECTED, CellularNetGetStatus());

    /* ========================================================
     * 2. Configurar y activar PDP
     * ======================================================== */

    TEST_ASSERT_EQUAL(ESP_OK, CellularNetConnectApn(
            "datos.personal.com",
            "datos",
            "datos"
        )
    );

    TEST_ASSERT_EQUAL(NET_STATUS_READY, CellularNetGetStatus());

    /* ========================================================
     * 3. Abrir conexión TCP
     * ======================================================== */
//nzmgf-190-183-23-94.run.pinggy-free.link:40309
    TEST_ASSERT_EQUAL( ESP_OK, CellularNetOpenTcp( "nzmgf-190-183-23-94.run.pinggy-free.link", 40309));

    TEST_ASSERT_EQUAL(NET_STATUS_SOCKET_CONNECTED, CellularNetGetStatus());

    /* ========================================================
     * 4. Enviar trama
     * ======================================================== */

    TEST_ASSERT_EQUAL(ESP_OK, CellularNetSendData(frame, strlen((char *)frame)));

    /* ========================================================
     * 5. Recibir respuesta
     * ======================================================== */

    TEST_ASSERT_EQUAL(ESP_OK, CellularNetReceiveData(rx_buffer,sizeof(rx_buffer), &received));

    printf("Datos recibidos: %u bytes\n", (unsigned)received);

    if (received > 0)
    {
        printf("Respuesta: %.*s\n",(int)received,rx_buffer);
    }

    /* ========================================================
     * 6. Cerrar conexión TCP
     * ======================================================== */

    TEST_ASSERT_EQUAL(ESP_OK, CellularNetCloseTcp());

    TEST_ASSERT_EQUAL(NET_STATUS_READY,  CellularNetGetStatus() );
}



#include "panic_handler.h"
#include "status_indicator.h"
#include "cellular_net.h"
#include "event_frame.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

TEST_CASE("Integridad HW: boton de panico + LED + modulo celular", "[cellular+led+boton]")
{
    /* 1. Inicializacion de las tres capas involucradas */
    TEST_ASSERT_TRUE_MESSAGE(PanicHandlerInit(), "Fallo init de PanicHandler (BSP boton GPIO23)");

    UartHalInit(UART_BAUDRATE);

    TEST_ASSERT_TRUE_MESSAGE(StatusIndicatorInit(), "Fallo init de StatusIndicator (LED_PANIC GPIO4 / LED_QUECTEL GPIO5)");
    //TEST_ASSERT_EQUAL(ESP_OK, CellularNetInit());

    /* Estado inicial esperado */
    TEST_ASSERT_EQUAL(PANIC_STATUS_NORMAL, PanicHandlerGetStatus());
    TEST_ASSERT_FALSE(PanicHandlerIsActive());
    StatusIndicatorSetCellular(CELLULAR_STATUS_STARTING);
    StatusIndicatorRunStep();

    printf(">>> Presione el boton de panico (GPIO23) para continuar...\n");

    /* 2. Esperar el flanco fisico real, con timeout, corriendo ambos RunStep */
    const int64_t timeout_ms = 10000;
    int64_t start_ms = esp_timer_get_time() / 1000;
    bool detected = false;

    while ((esp_timer_get_time() / 1000 - start_ms) < timeout_ms) {
        PanicHandlerRunStep();
        StatusIndicatorSetPanic(PanicHandlerIsActive());
        StatusIndicatorRunStep();

        if (PanicHandlerIsActive()) {
            detected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    TEST_ASSERT_TRUE_MESSAGE(detected, "No se detecto la pulsacion dentro del timeout");
    TEST_ASSERT_EQUAL(PANIC_STATUS_PENDING, PanicHandlerGetStatus());

    /* 3. Conexion celular real y transmision del evento */
    StatusIndicatorSetCellular(CELLULAR_STATUS_SEARCHING);
    StatusIndicatorRunStep();

    TEST_ASSERT_EQUAL(ESP_OK, CellularNetConnectApn("internet", "", ""));
    TEST_ASSERT_EQUAL(NET_STATUS_READY, CellularNetGetStatus());
    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);
    StatusIndicatorRunStep();

    TEST_ASSERT_TRUE(EventFrameInit());
    uint8_t frame[128];
    size_t frame_len = sizeof(frame);
    TEST_ASSERT_TRUE_MESSAGE(
        EventFrameBuild((uint8_t)PanicHandlerGetStatus(), "TEST_TOKEN", frame, &frame_len),
        "No se pudo construir la trama de evento"
    );

    StatusIndicatorSetCellular(CELLULAR_STATUS_TRANSMITTING);
    StatusIndicatorRunStep();

    bool sent = EventFrameDispatch(frame, frame_len);
    TEST_ASSERT_TRUE_MESSAGE(sent, "Fallo el envio de la trama (ni TCP ni SMS confirmaron)");

    /* 4. Confirmacion -> vuelta a estado normal en las tres capas */
    PanicHandlerClear();
    StatusIndicatorSetPanic(false);
    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);
    StatusIndicatorRunStep();

    TEST_ASSERT_EQUAL(PANIC_STATUS_NORMAL, PanicHandlerGetStatus());
    TEST_ASSERT_FALSE(PanicHandlerIsActive());
}