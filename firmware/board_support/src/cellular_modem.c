#include "cellular_modem.h"

#include "uart_hal.h"
#include "board_config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * Configuración interna
 * ============================================================ */

#define CELLULAR_AT_TIMEOUT_MS              1000
#define CELLULAR_BOOT_WAIT_MS               5000

#define CELLULAR_PWRKEY_STABILIZATION_MS    31
#define CELLULAR_PWRKEY_PULSE_ON_TIME_MS    2100

#define CELLULAR_SOCKET_ID                  0

#define CELLULAR_RX_BUFFER_SIZE             512

#define CELLULAR_TCP_TIMEOUT_MS             15000

/* ============================================================
 * Estado interno
 * ============================================================ */

/*
 * Si durante CellularModemSendTcp() llega también una URC
 * +QIURC: "recv", la guardamos para que ReceiveTcp() pueda
 * procesarla posteriormente.
 *
 * No es una cola ni un parser complejo.
 * Para este proyecto solamente necesitamos recordar
 * que hay datos disponibles en el socket.
 */
static bool s_rx_data_pending = false;
static size_t s_rx_bytes_pending = 0;

static uint8_t s_rx_data_buffer[256];
static size_t s_rx_data_length = 0;


/* ============================================================
 * Funciones privadas
 * ============================================================ */

static bool CellularModemPowerOn(void);

static bool CellularModemWaitFor(
    const char *expected,
    char *response,
    size_t response_size,
    uint32_t timeout_ms
);

static bool CellularModemProcessReceiveUrc(
    const char *response
);

/* ============================================================
 * Inicialización
 * ============================================================ */

bool CellularModemInit(void)
{
    UartHalInit(UART_BAUDRATE);
    printf("CELLULAR MODEM: UART initialized\n");
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);
    GPIOOff(QUECTEL_PWRKEY_PIN);

    if (!CellularModemPowerOn())
    {
        printf(
            "CELLULAR MODEM: Power on failed\n"
        );

        return false;
    }

    if (!CellularModemWaitReady(CELLULAR_BOOT_WAIT_MS))
    {
        printf(
            "CELLULAR MODEM: Modem not ready\n"
        );

        return false;
    }

    printf(
        "CELLULAR MODEM: Initialization complete\n"
    );

    return true;
}

/* ============================================================
 * Power
 * ============================================================ */

static bool CellularModemPowerOn(void)
{
    GPIOOff(QUECTEL_PWRKEY_PIN);
    HalDelayMs(CELLULAR_PWRKEY_STABILIZATION_MS);
    GPIOOn(QUECTEL_PWRKEY_PIN);
    HalDelayMs(CELLULAR_PWRKEY_PULSE_ON_TIME_MS);
    GPIOOff(QUECTEL_PWRKEY_PIN);
    return true;
}

/* ============================================================
 * Estado
 * ============================================================ */

bool CellularModemWaitReady(uint32_t timeout_ms)
{
    char response[CELLULAR_RX_BUFFER_SIZE];

    int len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        timeout_ms
    );

    if (len <= 0)
    {
        printf(
            "CELLULAR MODEM: RDY timeout\n"
        );

        return false;
    }

    response[len] = '\0';

    printf(
        "CELLULAR MODEM RX: %s\n",
        response
    );

    if (strstr(response, "RDY") != NULL)
    {
        printf(
            "CELLULAR MODEM: RDY received\n"
        );

        return true;
    }

    printf(
        "CELLULAR MODEM: RDY not found\n"
    );

    return false;
}


bool CellularModemIsReady(void)
{
    char response[128];

    return CellularModemSendCommand(
        "AT\r\n",
        response,
        sizeof(response),
        CELLULAR_AT_TIMEOUT_MS
    );
}

/* ============================================================
 * AT command
 * ============================================================ */

bool CellularModemSendCommand(
    const char *command,
    char *response,
    size_t response_size,
    uint32_t timeout_ms
)
{
    if (command == NULL ||
        response == NULL ||
        response_size == 0)
    {
        return false;
    }

    response[0] = '\0';

    /*
     * --------------------------------------------------------
     * TX
     * --------------------------------------------------------
     */

    int written = UartHalWriteBytes(
        command,
        strlen(command)
    );

    if (written <= 0)
    {
        printf(
            "CELLULAR MODEM: TX failed: %s",
            command
        );

        return false;
    }

    printf(
        "CELLULAR MODEM TX: %s",
        command
    );

    /*
     * --------------------------------------------------------
     * RX
     * --------------------------------------------------------
     */

    int len = UartHalReadBytes(
        response,
        response_size - 1,
        timeout_ms
    );

    if (len <= 0)
    {
        printf(
            "CELLULAR MODEM: RX timeout\n"
        );

        return false;
    }

    response[len] = '\0';

    printf(
        "CELLULAR MODEM RX: %s\n",
        response
    );

    /*
     * Si durante la respuesta apareció una URC de recepción,
     * no la descartamos.
     */
    CellularModemProcessReceiveUrc(response);

    /*
     * ERROR tiene prioridad sobre OK.
     */
    if (strstr(response, "ERROR") != NULL)
    {
        printf(
            "CELLULAR MODEM: AT command returned ERROR\n"
        );

        return false;
    }

    if (strstr(response, "OK") != NULL)
    {
        return true;
    }

    printf(
        "CELLULAR MODEM: Unexpected AT response\n"
    );

    return false;
}

/* ============================================================
 * WaitFor
 * ============================================================ */

/*
 * Envía NO comando.
 *
 * Solamente lee UART y espera encontrar una cadena determinada.
 *
 * Ejemplos:
 *
 *     CellularModemWaitFor("> ", ...)
 *
 *     CellularModemWaitFor("+QIURC: \"recv\"", ...)
 *
 *     CellularModemWaitFor("+QIOPEN:", ...)
 *
 * Es una herramienta interna para los comandos que no tienen
 * una respuesta AT convencional.
 */

static bool CellularModemWaitFor(
    const char *expected,
    char *response,
    size_t response_size,
    uint32_t timeout_ms
)
{
    if (expected == NULL ||
        response == NULL ||
        response_size == 0)
    {
        return false;
    }

    response[0] = '\0';

    int len = UartHalReadBytes(
        response,
        response_size - 1,
        timeout_ms
    );

    if (len <= 0)
    {
        return false;
    }

    response[len] = '\0';

    printf(
        "CELLULAR MODEM RX: %s\n",
        response
    );

    /*
     * Guardar URC de recepción si apareció junto
     * con la respuesta que estamos esperando.
     */
    CellularModemProcessReceiveUrc(response);

    if (strstr(response, expected) != NULL)
    {
        return true;
    }

    return false;
}

/* ============================================================
 * URC de recepción
 * ============================================================ */

static bool CellularModemProcessReceiveUrc(
    const char *response
)
{
    const char *p;

    if (response == NULL)
    {
        return false;
    }

    /*
     * Buscar:
     *
     * +QIURC: "recv",0,<cantidad>
     */
    p = strstr(
        response,
        "+QIURC: \"recv\""
    );

    if (p == NULL)
    {
        return false;
    }

    int socket_id;
    int bytes;

    if (sscanf(
            p,
            "+QIURC: \"recv\",%d,%d",
            &socket_id,
            &bytes
        ) != 2)
    {
        return false;
    }

    if (socket_id != CELLULAR_SOCKET_ID ||
        bytes <= 0)
    {
        return false;
    }

    s_rx_data_pending = true;
    s_rx_bytes_pending = (size_t)bytes;
    s_rx_data_length = 0;

    /*
     * Buscar el final de la línea de la URC.
     *
     * Ejemplo:
     *
     * +QIURC: "recv",0,4\r\n
     * ACK
     */
    const char *data_start = strstr(
        p,
        "\r\n"
    );

    if (data_start != NULL)
    {
        data_start += 2;

        /*
         * Determinar cuántos bytes quedaron después
         * de la línea de la URC.
         */
        size_t offset = (size_t)(
            data_start - response
        );

        size_t response_length = strlen(response);

        size_t available =
            response_length - offset;

        /*
         * Si ya llegaron los datos junto con la URC,
         * guardarlos.
         */
        if (available >= (size_t)bytes)
        {
            if ((size_t)bytes <= sizeof(s_rx_data_buffer))
            {
                memcpy(
                    s_rx_data_buffer,
                    data_start,
                    (size_t)bytes
                );

                s_rx_data_length = (size_t)bytes;

                printf(
                    "CELLULAR MODEM: RX data already available "
                    "(%zu bytes)\n",
                    s_rx_data_length
                );
            }
        }
    }

    printf(
        "CELLULAR MODEM: RX data pending "
        "socket=%d bytes=%d\n",
        socket_id,
        bytes
    );

    return true;
}
/* ============================================================
 * IMEI
 * ============================================================ */

bool CellularModemGetImei(
    char *imei,
    size_t imei_size
)
{
    if (imei == NULL ||
        imei_size == 0)
    {
        return false;
    }

    char response[128];

    if (!CellularModemSendCommand(
            "AT+CGSN\r\n",
            response,
            sizeof(response),
            2000))
    {
        return false;
    }

    /*
     * Buscar una secuencia de dígitos.
     *
     * La respuesta esperada es:
     *
     * 869671077009056
     *
     * OK
     */

    const char *p = response;

    while (*p != '\0')
    {
        if (*p >= '0' && *p <= '9')
        {
            const char *start = p;

            while (*p >= '0' && *p <= '9')
            {
                p++;
            }

            size_t length = (size_t)(p - start);

            if (length > 0 &&
                length < imei_size)
            {
                memcpy(
                    imei,
                    start,
                    length
                );

                imei[length] = '\0';

                return true;
            }
        }
        else
        {
            p++;
        }
    }

    return false;
}

/* ============================================================
 * PDP
 * ============================================================ */

bool CellularModemConfigurePdp(
    const char *apn,
    const char *username,
    const char *password
)
{
    if (apn == NULL ||
        username == NULL ||
        password == NULL)
    {
        return false;
    }

    char command[192];
    char response[256];

    snprintf(
        command,
        sizeof(command),
        "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n",
        apn,
        username,
        password
    );

    return CellularModemSendCommand(
        command,
        response,
        sizeof(response),
        5000
    );
}


bool CellularModemActivatePdp(void)
{
    char response[256];

    /*
     * Si ya está activo, no hace falta volver a activarlo.
     */
    if (CellularModemIsPdpActive())
    {
        printf(
            "CELLULAR MODEM: PDP already active\n"
        );

        return true;
    }

    return CellularModemSendCommand(
        "AT+QIACT=1\r\n",
        response,
        sizeof(response),
        15000
    );
}


bool CellularModemIsPdpActive(void)
{
    char response[256];

    if (!CellularModemSendCommand(
            "AT+QIACT?\r\n",
            response,
            sizeof(response),
            2000))
    {
        return false;
    }

    /*
     * Para el contexto 1 esperamos:
     *
     * +QIACT: 1,1,1,"IP"
     */
    if (strstr(
            response,
            "+QIACT: 1,1,1"
        ) != NULL)
    {
        return true;
    }

    return false;
}

/* ============================================================
 * TCP - Open
 * ============================================================ */

bool CellularModemOpenTcp(
    const char *server,
    uint16_t port
)
{
    if (server == NULL)
    {
        return false;
    }

    char command[192];
    char response[256];

    snprintf(
        command,
        sizeof(command),
        "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%u,0,1\r\n",
        CELLULAR_SOCKET_ID,
        server,
        port
    );

    /*
     * QIOPEN primero devuelve:
     *
     * OK
     *
     * y luego:
     *
     * +QIOPEN: 0,0
     *
     * El resultado importante es +QIOPEN: 0,0.
     */

    int written = UartHalWriteBytes(
        command,
        strlen(command)
    );

    if (written <= 0)
    {
        printf(
            "CELLULAR MODEM: QIOPEN TX failed\n"
        );

        return false;
    }

    printf(
        "CELLULAR MODEM TX: %s",
        command
    );

    /*
     * Primera respuesta: OK.
     */
    int len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        5000
    );

    if (len <= 0)
    {
        printf(
            "CELLULAR MODEM: QIOPEN response timeout\n"
        );

        return false;
    }

    response[len] = '\0';

    printf(
        "CELLULAR MODEM RX: %s\n",
        response
    );

    CellularModemProcessReceiveUrc(response);

    if (strstr(response, "ERROR") != NULL)
    {
        return false;
    }

    /*
     * Si ya vino +QIOPEN en la misma lectura.
     */
    if (strstr(response, "+QIOPEN:") != NULL)
    {
        if (strstr(response, "+QIOPEN: 0,0") != NULL)
        {
            return true;
        }

        return false;
    }

    /*
     * Si solamente llegó OK, esperar +QIOPEN.
     */
    len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        CELLULAR_TCP_TIMEOUT_MS
    );

    if (len <= 0)
    {
        printf(
            "CELLULAR MODEM: QIOPEN URC timeout\n"
        );

        return false;
    }

    response[len] = '\0';

    printf(
        "CELLULAR MODEM RX: %s\n",
        response
    );

    CellularModemProcessReceiveUrc(response);

    if (strstr(
            response,
            "+QIOPEN: 0,0"
        ) != NULL)
    {
        return true;
    }

    return false;
}

/* ============================================================
 * TCP - Send
 * ============================================================ */

bool CellularModemSendTcp(
    const uint8_t *data,
    size_t length
)
{
    if (data == NULL ||
        length == 0)
    {
        return false;
    }

    char command[64];
    char response[CELLULAR_RX_BUFFER_SIZE];

    /*
     * --------------------------------------------------------
     * Paso 1: solicitar prompt
     * --------------------------------------------------------
     */

    snprintf(
        command,
        sizeof(command),
        "AT+QISEND=%d,%zu\r\n",
        CELLULAR_SOCKET_ID,
        length
    );

    int written = UartHalWriteBytes(
        command,
        strlen(command)
    );

    if (written <= 0)
    {
        printf(
            "CELLULAR MODEM: QISEND TX failed\n"
        );

        return false;
    }

    printf(
        "CELLULAR MODEM TX: %s",
        command
    );

    /*
     * Esperar:
     *
     * >
     */
    int len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        5000
    );

    if (len <= 0)
    {
        printf(
            "CELLULAR MODEM: QISEND prompt timeout\n"
        );

        return false;
    }

    response[len] = '\0';

    printf(
        "CELLULAR MODEM RX: %s\n",
        response
    );

    if (strstr(response, "> ") == NULL)
    {
        printf(
            "CELLULAR MODEM: QISEND prompt not received\n"
        );

        return false;
    }

    /*
     * --------------------------------------------------------
     * Paso 2: enviar payload
     * --------------------------------------------------------
     */

    printf(
        "CELLULAR MODEM TX payload (%zu bytes): ",
        length
    );

    /*
     * El payload puede no ser texto.
     * Por eso no usamos %s.
     */
    fwrite(
        data,
        1,
        length,
        stdout
    );

    printf("\n");

    written = UartHalWriteBytes(
        (const char *)data,
        length
    );

    if (written != (int)length)
    {
        printf(
            "CELLULAR MODEM: Payload TX failed\n"
        );

        return false;
    }

    /*
     * --------------------------------------------------------
     * Paso 3: esperar SEND OK
     * --------------------------------------------------------
     */

    len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        5000
    );

    if (len <= 0)
    {
        printf(
            "CELLULAR MODEM: SEND OK timeout\n"
        );

        return false;
    }

    response[len] = '\0';

    printf(
        "CELLULAR MODEM RX: %s\n",
        response
    );

    /*
     * MUY IMPORTANTE:
     *
     * Puede llegar:
     *
     * SEND OK
     * +QIURC: "recv",0,3
     *
     * en la misma lectura.
     *
     * ProcessReceiveUrc() guarda esa información.
     */
    CellularModemProcessReceiveUrc(response);

    if (strstr(response, "SEND FAIL") != NULL)
    {
        printf(
            "CELLULAR MODEM: SEND FAIL\n"
        );

        return false;
    }

    if (strstr(response, "SEND OK") == NULL)
    {
        printf(
            "CELLULAR MODEM: SEND OK not received\n"
        );

        return false;
    }

    return true;
}

/* ============================================================
 * TCP - Receive
 * ============================================================ */

bool CellularModemReceiveTcp(
    uint8_t *data,
    size_t data_size,
    size_t *received
)
{
    if (data == NULL ||
        data_size == 0 ||
        received == NULL)
    {
        return false;
    }

    *received = 0;

    // ✅ Faltaba declarar este buffer
    char response[CELLULAR_RX_BUFFER_SIZE];

    /*
     * Si los datos ya llegaron junto con la URC,
     * devolverlos directamente.
     */
    if (s_rx_data_length > 0)
    {
        if (s_rx_data_length > data_size)
        {
            printf(
                "CELLULAR MODEM: RX buffer too small\n"
            );

            return false;
        }

        memcpy(
            data,
            s_rx_data_buffer,
            s_rx_data_length
        );

        *received = s_rx_data_length;

        s_rx_data_pending = false;
        s_rx_bytes_pending = 0;
        s_rx_data_length = 0;

        printf(
            "CELLULAR MODEM: RX data returned directly "
            "(%zu bytes)\n",
            *received
        );

        return true;
    }

    /*
     * Paso 1: verificar si ya sabemos que hay datos.
     */
    if (!s_rx_data_pending)
    {
        int len = UartHalReadBytes(
            response,
            sizeof(response) - 1,
            CELLULAR_TCP_TIMEOUT_MS
        );

        if (len <= 0)
        {
            printf(
                "CELLULAR MODEM: Receive timeout\n"
            );

            return false;
        }

        response[len] = '\0';

        printf(
            "CELLULAR MODEM RX: %s\n",
            response
        );

        if (!CellularModemProcessReceiveUrc(response))
        {
            printf(
                "CELLULAR MODEM: No receive URC found\n"
            );

            return false;
        }
    }

    /*
     * Paso 2: sabemos cuántos bytes hay.
     */
    size_t bytes_to_read = s_rx_bytes_pending;
    if (bytes_to_read > data_size)
    {
        bytes_to_read = data_size;
    }

    /*
     * Paso 3: QIRD
     */
    char command[64];
    snprintf(
        command,
        sizeof(command),
        "AT+QIRD=%d,%zu\r\n",
        CELLULAR_SOCKET_ID,
        bytes_to_read
    );

    int written = UartHalWriteBytes(
        command,
        strlen(command)
    );

    if (written <= 0)
    {
        printf(
            "CELLULAR MODEM: QIRD TX failed\n"
        );

        return false;
    }

    printf(
        "CELLULAR MODEM TX: %s",
        command
    );

    int len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        5000
    );

    if (len <= 0)
    {
        printf(
            "CELLULAR MODEM: QIRD timeout\n"
        );

        return false;
    }

    response[len] = '\0';

    printf(
        "CELLULAR MODEM RX: %s\n",
        response
    );

    const char *qird = strstr(response, "+QIRD:");
    if (qird == NULL)
    {
        printf(
            "CELLULAR MODEM: QIRD response invalid\n"
        );

        return false;
    }

    int actual_bytes = 0;
    if (sscanf(qird, "+QIRD: %d", &actual_bytes) != 1)
    {
        printf(
            "CELLULAR MODEM: Cannot parse QIRD length\n"
        );

        return false;
    }

    if (actual_bytes <= 0)
    {
        s_rx_data_pending = false;
        s_rx_bytes_pending = 0;
        return true;
    }

    if ((size_t)actual_bytes > data_size)
    {
        printf(
            "CELLULAR MODEM: RX buffer too small "
            "(needed=%d available=%zu)\n",
            actual_bytes,
            data_size
        );

        return false;
    }

    const char *data_start = strstr(qird, "\r\n");
    if (data_start == NULL)
    {
        printf(
            "CELLULAR MODEM: QIRD data not found\n"
        );

        return false;
    }

    data_start += 2;

    size_t offset = (size_t)(data_start - response);
    if (offset + (size_t)actual_bytes > (size_t)len)
    {
        printf(
            "CELLULAR MODEM: QIRD data incomplete\n"
        );

        return false;
    }

    memcpy(data, data_start, (size_t)actual_bytes);
    *received = (size_t)actual_bytes;

    s_rx_data_pending = false;
    s_rx_bytes_pending = 0;

    return true;
}

/* ============================================================
 * TCP - Close
 * ============================================================ */

bool CellularModemCloseTcp(void)
{
    char response[128];

    bool result = CellularModemSendCommand(
        "AT+QICLOSE=0\r\n",
        response,
        sizeof(response),
        10000
    );

    /*
     * Al cerrar el socket ya no debe quedar una recepción
     * pendiente asociada al mismo.
     */
    s_rx_data_pending = false;
    s_rx_bytes_pending = 0;

    return result;
}