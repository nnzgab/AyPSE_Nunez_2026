#include "unity.h"
#include "esp_err.h"
#include "uart_hal.h"
#include "cellular_modem.h"


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


#define UART_BAUDRATE 115200

#include "event_frame.h"
#include <string.h>
#include <stdint.h>

TEST_CASE("tests del middleware", "[prueba_frame]")
{
    
    /* 1. Inicializar la UART y el módem para poder consultar el IMEI */
    UartHalInit(UART_BAUDRATE);
    
    // Opcional: aseguramos que el módem esté inicializado o listo si corresponde
    //CellularModemInit();

    /* 2. Obtener el IMEI real desde el BSP del módem */
    char imei_buffer[32] = {0};
    bool imei_success = CellularModemGetImei(imei_buffer, sizeof(imei_buffer));
    
    TEST_ASSERT_TRUE_MESSAGE(imei_success, "No se pudo obtener el IMEI del modulo celular");
    TEST_ASSERT_TRUE_MESSAGE(strlen(imei_buffer) > 0, "El IMEI obtenido esta vacio");

    /* 3. Construir la estructura del evento con el IMEI dinámico */
    event_frame_t evt = {
        .type = EVENT_TYPE_PANIC,
    };
    
    // Copiamos el IMEI real obtenido del hardware a la estructura
    strncpy(evt.imei, imei_buffer, sizeof(evt.imei) - 1);
    evt.imei[sizeof(evt.imei) - 1] = '\0';

    /* 4. Construir la trama */
    char frame[64] = {0};
    int len = EventFrameBuild(&evt, frame, sizeof(frame));

    TEST_ASSERT_TRUE_MESSAGE(len > 0, "No se pudo construir la trama de evento");
}

#include "panic_handler.h"
#include "status_indicator.h"
#include "cellular_net.h"
#include "event_frame.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* TODO: reemplazar por la lectura real del IMEI cuando esté disponible */
#define TEST_IMEI         "864920040123456"
#define TEST_APN          "internet"
#define TEST_SERVER_IP    "TU_HOST_PINGGY"   /* actualizar: el tunel es efimero */
#define TEST_SERVER_PORT  12345
#define TEST_ACK_STR      "ACK"

TEST_CASE("Integridad HW: encendido modulo + boton + alarma por TCP", "[fullfull]")
{
    /* 1. Init de las tres capas */
    TEST_ASSERT_TRUE_MESSAGE(PanicHandlerInit(), "Fallo init de PanicHandler");
    TEST_ASSERT_TRUE_MESSAGE(StatusIndicatorInit(), "Fallo init de StatusIndicator");

    StatusIndicatorSetCellular(CELLULAR_STATUS_STARTING);
    StatusIndicatorRunStep();
    TEST_ASSERT_EQUAL(ESP_OK, CellularNetInit());

    /* 2. Obtener el IMEI real desde el BSP del módem */
    char imei_buffer[32] = {0};
    TEST_ASSERT_TRUE_MESSAGE(CellularModemGetImei(imei_buffer, sizeof(imei_buffer)), "No se pudo obtener el IMEI del modulo celular");

    /* 2. Esperar registro en la red */
    StatusIndicatorSetCellular(CELLULAR_STATUS_SEARCHING);
    StatusIndicatorRunStep();

    const int64_t reg_timeout_ms = 30000;
    int64_t start_ms = esp_timer_get_time() / 1000;
    bool registered = false;

    while ((esp_timer_get_time() / 1000 - start_ms) < reg_timeout_ms)
    {
        StatusIndicatorRunStep();
        if (CellularNetGetStatus() != NET_STATUS_DISCONNECTED)
        {
            registered = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    TEST_ASSERT_TRUE_MESSAGE(registered, "El modulo no registro en la red dentro del timeout");

    /* 3. Activar contexto PDP */
    TEST_ASSERT_EQUAL(ESP_OK, CellularNetConnectApn(TEST_APN, "", ""));
    TEST_ASSERT_EQUAL(NET_STATUS_READY, CellularNetGetStatus());
    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);
    StatusIndicatorRunStep();

    /* 4. Esperar la pulsacion fisica del boton */
    printf(">>> Modulo listo. Presione el boton de panico (GPIO23)...\n");
    const int64_t btn_timeout_ms = 10000;
    start_ms = esp_timer_get_time() / 1000;
    bool detected = false;

    while ((esp_timer_get_time() / 1000 - start_ms) < btn_timeout_ms)
    {
        PanicHandlerRunStep();
        StatusIndicatorSetPanic(PanicHandlerIsActive());
        StatusIndicatorRunStep();
        if (PanicHandlerIsActive())
        {
            detected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    TEST_ASSERT_TRUE_MESSAGE(detected, "No se detecto la pulsacion del boton");
    TEST_ASSERT_EQUAL(PANIC_STATUS_PENDING, PanicHandlerGetStatus());

    /* 5. Armar la trama con el IMEI dinámico (copia segura sin truncamiento) */
    event_frame_t evt = {
        .type = EVENT_TYPE_PANIC,
    };
    
    size_t imei_len = strlen(imei_buffer);
    if (imei_len >= sizeof(evt.imei)) {
        imei_len = sizeof(evt.imei) - 1;
    }
    memcpy(evt.imei, imei_buffer, imei_len);
    evt.imei[imei_len] = '\0';

    char frame[64];
    int frame_len = EventFrameBuild(&evt, frame, sizeof(frame));
    TEST_ASSERT_TRUE_MESSAGE(frame_len > 0, "No se pudo construir la trama de evento");

    /* 6. Disparar la alarma: abrir TCP, enviar, esperar ACK, cerrar */
    StatusIndicatorSetCellular(CELLULAR_STATUS_TRANSMITTING);
    StatusIndicatorRunStep();

    TEST_ASSERT_EQUAL(ESP_OK, CellularNetOpenTcp(TEST_SERVER_IP, TEST_SERVER_PORT));
    TEST_ASSERT_EQUAL(NET_STATUS_SOCKET_CONNECTED, CellularNetGetStatus());
    TEST_ASSERT_EQUAL(ESP_OK, CellularNetSendData((const uint8_t *)frame, (size_t)frame_len));

    uint8_t rx_buf[32] = {0};
    size_t received = 0;
    esp_err_t rx_err = ESP_FAIL;

    const int64_t ack_timeout_ms = 5000;
    start_ms = esp_timer_get_time() / 1000;
    while ((esp_timer_get_time() / 1000 - start_ms) < ack_timeout_ms)
    {
        rx_err = CellularNetReceiveData(rx_buf, sizeof(rx_buf) - 1, &received);
        if (rx_err == ESP_OK && received > 0)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, rx_err, "No se recibio respuesta del servidor");
    TEST_ASSERT_TRUE_MESSAGE(received > 0, "Respuesta vacia del servidor");
    rx_buf[received] = '\0';
    TEST_ASSERT_EQUAL_STRING_MESSAGE(TEST_ACK_STR, (const char *)rx_buf, "El servidor no confirmo con ACK");

    CellularNetCloseTcp();

    /* 7. Confirmado -> limpiar alarma, LEDs a estado normal */
    PanicHandlerClear();
    StatusIndicatorSetPanic(false);
    StatusIndicatorSetCellular(CELLULAR_STATUS_READY);
    StatusIndicatorRunStep();

    TEST_ASSERT_EQUAL(PANIC_STATUS_NORMAL, PanicHandlerGetStatus());
    TEST_ASSERT_FALSE(PanicHandlerIsActive());
}