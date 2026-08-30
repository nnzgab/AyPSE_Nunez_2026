#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "cellular.h"
#include "uart_hal.h"
#include "board_config.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Configuración interna
 * ============================================================ */


#define CELLULAR_PWRKEY_STABILIZATION_MS 31
#define CELLULAR_PWRKEY_PULSE_ON_TIME_MS 2100
#define CELLULAR_PWRKEY_ON_TIME_MS       100
#define CELLULAR_PWRKEY_OFF_TIME_MS      2100
#define CELLULAR_BOOT_WAIT_MS            5000

#define CELLULAR_AT_TIMEOUT_MS           1000
#define CELLULAR_POWEROFF_TIMEOUT_MS     5000
#define CELLULAR_CFUN_TIMEOUT_MS    10000

#define CELLULAR_NETWORK_POLL_MS       1000
#define CELLULAR_NETWORK_TIMEOUT_MS    5000//120000
#define CELLULAR_QIACT_TIMEOUT_MS    5000//160000


#define CELLULAR_QIOPEN_TIMEOUT_MS    15000//60000  // apertura de socket
#define CELLULAR_QIOPEN_COMMAND_TIMEOUT_MS    5000
#define CELLULAR_QIOPEN_RESULT_TIMEOUT_MS    5000//30000
#define CELLULAR_QICSGP_TIMEOUT_MS    2000   // configuración PDP

#define CELLULAR_QICLOSE_TIMEOUT_MS   2000   // cierre de socket

#define CELLULAR_TCP_COMMAND_TIMEOUT_MS 25000

#define CELLULAR_PWRKEY_POWEROFF_PULSE_MS 3000

#define CELLULAR_QPOWD_TIMEOUT_MS 5000//20000


/* ============================================================
 * AT RX Engine
 * ============================================================
 *                     UART RX
 *                        │
 *                        ▼
 *              ┌───────────────────┐
 *              │   AT RX ENGINE     │   CellularEngineStep()
 *              └─────────┬──────────┘
 *                        │
 *              ┌─────────┴─────────┐
 *              ▼                   ▼
 *          RESPUESTA              URC
 *              │                   │
 *              │                   ▼
 *              │              ┌─────────┐
 *              │              │  QUEUE  │
 *              │              └────┬────┘
 *              ▼                   ▼
 *        SendCommand           ReceiveTcp /
 *                              WaitForResponse
 *
 * CellularEngineStep() es el ÚNICO punto de contacto con
 * UartHalReadBytes(). Lee lo que haya disponible y clasifica cada
 * línea completa en una de dos categorías:
 *
 *   - RESPUESTA: pertenece a la respuesta síncrona del comando AT en
 *     curso (ej: "OK", "+CSQ: 20,99", "SEND OK", ">"). Se devuelve tal
 *     cual al que llamó (normalmente CellularSendCommand).
 *
 *   - URC (Unsolicited Result Code): evento asíncrono del módem que
 *     puede llegar en cualquier momento, incluso pegado a la
 *     respuesta de un comando que no tiene nada que ver (el caso que
 *     nos rompía los tests: "+QIURC: \"recv\",0,5" llegando junto con
 *     "SEND OK"). Se encola en s_urc_queue en vez de mezclarse con la
 *     respuesta, y se consume después por prefijo/contenido desde
 *     donde haga falta.
 *
 * Por qué esto es genérico y no un parche puntual para QIURC:
 * cualquier línea que matchee un prefijo de la tabla s_urc_prefixes
 * se trata igual, sin importar qué función disparó la lectura física
 * que la trajo. Agregar un URC nuevo en el futuro es una línea en esa
 * tabla, no una modificación en cada función que pueda toparse con él.
 *
 * ALCANCE ACTUAL: solo "+QIURC:" está en la tabla. "+QIOPEN:" queda
 * afuera a propósito por ahora — CellularOpenSocket() todavía espera
 * verlo mezclado en la respuesta de CellularSendCommand(), como
 * siempre. El día que se quiera hacer lo mismo con QIOPEN, hay que
 * agregarlo a la tabla Y adaptar CellularOpenSocket() para que lo
 * consuma de la cola en vez de parsearlo de 'response' — las dos
 * cosas juntas, no una sin la otra.
 *
 * No hay una tarea de FreeRTOS leyendo en background: el modelo es
 * "pull" — cada consumidor dispara una lectura física cuando la
 * necesita. Como los tests corren secuencialmente (nunca hay dos
 * comandos en vuelo al mismo tiempo), esto alcanza sin necesitar
 * concurrencia real. Si el día de mañana hiciera falta un listener
 * continuo, este mismo clasificador se reutilizaría dentro de esa
 * tarea sin cambios.
 */

#define CELLULAR_URC_QUEUE_SLOTS     4
#define CELLULAR_URC_ITEM_SIZE       128
#define CELLULAR_ENGINE_LINE_SIZE    128

typedef struct
{
    char text[CELLULAR_URC_ITEM_SIZE];
    bool used;
} CellularUrcSlot;

static CellularUrcSlot s_urc_queue[CELLULAR_URC_QUEUE_SLOTS];

/*
 * Prefijos que identifican una línea como URC en vez de respuesta de
 * comando. Ver nota de "ALCANCE ACTUAL" arriba antes de agregar
 * "+QIOPEN:" acá.
 */
static const char *const s_urc_prefixes[] = {
    "+QIURC:",
};
#define CELLULAR_URC_PREFIX_COUNT (sizeof(s_urc_prefixes) / sizeof(s_urc_prefixes[0]))

static bool CellularLineIsUrc(const char *line)
{
    for (size_t i = 0; i < CELLULAR_URC_PREFIX_COUNT; i++)
    {
        if (strncmp(line, s_urc_prefixes[i], strlen(s_urc_prefixes[i])) == 0)
        {
            return true;
        }
    }
    return false;
}

static void CellularUrcEnqueue(const char *line)
{
    for (int i = 0; i < CELLULAR_URC_QUEUE_SLOTS; i++)
    {
        if (!s_urc_queue[i].used)
        {
            strncpy(s_urc_queue[i].text, line, CELLULAR_URC_ITEM_SIZE - 1);
            s_urc_queue[i].text[CELLULAR_URC_ITEM_SIZE - 1] = '\0';
            s_urc_queue[i].used = true;
            printf("CELLULAR: URC encolado: %s\n", line);
            return;
        }
    }

    printf("CELLULAR: Cola de URCs llena, se descarta: %s\n", line);
}

/*
 * Busca el primer URC en cola cuyo texto contenga 'expected' y lo
 * extrae (dequeue). No importa en qué llamada a CellularEngineStep
 * haya llegado: pudo ser durante un CellularSendCommand de otro
 * comando, o durante una espera anterior que no lo estaba buscando.
 */
static bool CellularUrcDequeueMatching(
    const char *expected,
    char *out,
    size_t out_size
)
{
    for (int i = 0; i < CELLULAR_URC_QUEUE_SLOTS; i++)
    {
        if (s_urc_queue[i].used && strstr(s_urc_queue[i].text, expected) != NULL)
        {
            strncpy(out, s_urc_queue[i].text, out_size - 1);
            out[out_size - 1] = '\0';
            s_urc_queue[i].used = false;
            printf("CELLULAR: URC desencolado: %s\n", out);
            return true;
        }
    }

    return false;
}

/*
 * Único punto de contacto con UartHalReadBytes(). Hace UNA lectura
 * física (mismo comportamiento de timeout que antes), separa el
 * resultado por líneas ("\r\n") y clasifica cada una:
 *
 *   - Si matchea un prefijo URC -> se encola, nunca llega a
 *     response_out.
 *   - Si no -> se agrega a response_out, conservando los "\r\n"
 *     originales (varios parsers existentes, como el de QIRD, hacen
 *     strchr(buffer, '\n') sobre el bloque completo, así que no hay
 *     que cambiarles el formato que reciben).
 *
 * response_out puede ser NULL (o response_out_size 0) cuando a quien
 * llama solo le interesa que los URCs que hayan llegado queden
 * encolados, y no le importa el resto del texto (es el caso de
 * CellularWaitForResponse mientras "bombea" UART buscando un URC).
 *
 * Devuelve lo mismo que UartHalReadBytes: bytes físicos leídos, 0 si
 * no llegó nada en el timeout, negativo en error de lectura.
 */
static int CellularEngineStep(
    uint32_t timeout_ms,
    char *response_out,
    size_t response_out_size
)
{
    char raw[256];

    int len = UartHalReadBytes(raw, sizeof(raw) - 1, timeout_ms);

    if (len <= 0)
    {
        return len;
    }

    raw[len] = '\0';

    char *line_start = raw;

    while (*line_start != '\0')
    {
        char *line_end = strstr(line_start, "\r\n");
        bool has_delim = (line_end != NULL);
        size_t line_len = has_delim
            ? (size_t)(line_end - line_start)
            : strlen(line_start);

        if (line_len > 0)
        {
            char line_buf[CELLULAR_ENGINE_LINE_SIZE];
            size_t copy_len = (line_len < sizeof(line_buf) - 1)
                ? line_len
                : sizeof(line_buf) - 1;

            memcpy(line_buf, line_start, copy_len);
            line_buf[copy_len] = '\0';

            if (CellularLineIsUrc(line_buf))
            {
                CellularUrcEnqueue(line_buf);
            }
            else if (response_out != NULL && response_out_size > 0)
            {
                size_t current = strlen(response_out);

                if (current + copy_len + 2 < response_out_size)
                {
                    memcpy(response_out + current, line_start, copy_len);
                    current += copy_len;
                    response_out[current] = '\0';

                    if (has_delim)
                    {
                        strcat(response_out, "\r\n");
                    }
                }
            }
        }

        if (!has_delim)
        {
            break;
        }

        line_start = line_end + 2; // saltar "\r\n"
    }

    return len;
}


bool CellularPowerOn(void)
{
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);

    // Asegurar línea liberada (no presionado) antes de iniciar
    GPIOOff(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(CELLULAR_PWRKEY_STABILIZATION_MS)); // >= 30 ms

    // Presionar PWRKEY (HIGH -> transistor satura -> PWRKEY a GND)
    GPIOOn(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(CELLULAR_PWRKEY_PULSE_ON_TIME_MS)); // >= 2000 ms

    // Soltar PWRKEY (LOW -> transistor corta -> PWRKEY flota a alto vía pull-up)
    GPIOOff(QUECTEL_PWRKEY_PIN);

    return true;
}


bool CellularWaitReady(uint32_t timeout_ms)/************* */
{
    char response[256];

    int len = UartHalReadBytes(response, sizeof(response) - 1, timeout_ms);

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';

    printf("CELLULAR RX: %s\n", response);

    return strstr(response, "RDY") != NULL;
}


bool CellularIsReady(void)
{
    const char *command = "AT\r\n";

    int written = UartHalWriteBytes(command, strlen(command));
    if (written <= 0) {
        printf("CELLULAR AT TX failed\n");
        return false;
    }

    char response[256];
    int len = UartHalReadBytes(
        response,
        sizeof(response) - 1,
        CELLULAR_AT_TIMEOUT_MS
    );

    if (len <= 0) {
        return false;
    }

    response[len] = '\0';
    printf("CELLULAR AT RX: %s\n", response);

    return strstr(response, "OK") != NULL;
}


/* ============================================================
 * Envío de comandos AT
 * ============================================================ 
 * */


bool CellularSendCommand(
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

    /*
     * Limpiamos el buffer antes de utilizarlo.
     */
    response[0] = '\0';

    /*
     * Enviamos el comando al EG915U.
     */
    int written = UartHalWriteBytes(command, strlen(command));

    if (written <= 0)
    {
        printf("CELLULAR TX failed: %s", command);
        return false;
    }

    printf("CELLULAR TX: %s", command);

    /*
     * Esperamos la respuesta. Pasa por el AT RX Engine: si en esta
     * misma lectura física llegó algo pegado que matchea un prefijo
     * URC (hoy: "+QIURC:"), el engine lo desvía a la cola en vez de
     * dejarlo mezclado acá adentro. 'response' solo recibe las
     * líneas que NO son URC.
     */
    int len = CellularEngineStep(
        timeout_ms,
        response,
        response_size
    );

    if (len <= 0)
    {
        printf("CELLULAR RX timeout\n");
        return false;
    }

    printf("CELLULAR RX: %s\n", response);

    return true;
}



bool CellularEchoOff(void)
{
    char response[128];

    if (!CellularSendCommand(
            "ATE0\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    return strstr(response, "OK") != NULL;
}

bool CellularSetFullFunction(void)
{
    char response[128];

    if (!CellularSendCommand(
            "AT+CFUN=1\r\n",
            response,
            sizeof(response),
            CELLULAR_CFUN_TIMEOUT_MS))
    {
        return false;
    }

    return strstr(response, "OK") != NULL;
}


bool CellularGetIMSI(char *imsi, size_t imsi_size)
{
    char response[128];

    if (imsi == NULL || imsi_size == 0)
    {
        return false;
    }

    if (!CellularSendCommand(
            "AT+CIMI\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    /*
     * Buscamos el IMSI dentro de la respuesta.
     */
    char *start = response;

    while (*start == '\r' || *start == '\n')
    {
        start++;
    }

    /*
     * Copiamos hasta CR/LF.
     */
    size_t i = 0;

    while (start[i] != '\0' &&
           start[i] != '\r' &&
           start[i] != '\n' &&
           i < imsi_size - 1)
    {
        imsi[i] = start[i];
        i++;
    }

    imsi[i] = '\0';

    /*
     * Un IMSI normalmente tiene 15 dígitos.
     */
    if (i != 15)
    {
        printf("CELLULAR: Invalid IMSI length: %d\n", (int)i);
        return false;
    }

    /*
     * Verificamos que todos sean dígitos.
     */
    for (i = 0; i < 15; i++)
    {
        if (imsi[i] < '0' || imsi[i] > '9')
        {
            printf("CELLULAR: Invalid IMSI\n");
            return false;
        }
    }

    printf("CELLULAR IMSI: %s\n", imsi);

    return true;
}

bool CellularWaitNetworkRegistration(uint32_t timeout_ms)
{
    uint32_t elapsed_ms = 0;

    while (elapsed_ms < timeout_ms)
    {
        int status;

        if (!CellularGetNetworkRegistration(&status))
        {
            printf("CELLULAR: Failed to get network registration\n");
        }
        else
        {
            printf(
                "CELLULAR: Registration status = %d\n",
                status
            );

            /*
             * 1 = registrado en red local
             * 5 = registrado en roaming
             */
            if (status == 1 || status == 5)
            {
                printf("CELLULAR: Network registered!\n");
                return true;
            }

            /*
             * 3 = registro rechazado.
             *
             * No tiene sentido seguir esperando
             * indefinidamente en este caso.
             */
            if (status == 3)
            {
                printf("CELLULAR: Network registration rejected\n");
                return false;
            }
        }

        vTaskDelay(
            pdMS_TO_TICKS(CELLULAR_NETWORK_POLL_MS)
        );

        elapsed_ms += CELLULAR_NETWORK_POLL_MS;
    }

    printf("CELLULAR: Network registration timeout\n");

    return false;
}


bool CellularGetNetworkRegistration(int *status)
{
    char response[128];

    if (status == NULL)
    {
        return false;
    }

    if (!CellularSendCommand(
            "AT+CEREG?\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    /*
     * Buscamos la respuesta +CEREG.
     *
     * Ejemplo:
     * +CEREG: 0,2
     * OK
     */
    char *cereg = strstr(response, "+CEREG:");

    if (cereg == NULL)
    {
        printf("CELLULAR: CEREG response not found\n");
        return false;
    }

    /*
     * Buscamos la coma.
     *
     * +CEREG: 0,2
     *          ^
     */
    char *comma = strchr(cereg, ',');

    if (comma == NULL)
    {
        printf("CELLULAR: Invalid CEREG response\n");
        return false;
    }

    /*
     * El estado está después de la coma.
     *
     * +CEREG: 0,2
     *            ^
     */
    int registration_status;

    if (sscanf(comma + 1, "%d", &registration_status) != 1)
    {
        printf("CELLULAR: Cannot parse CEREG status\n");
        return false;
    }

    *status = registration_status;

    printf(
        "CELLULAR: Network registration status = %d\n",
        *status
    );

    return true;
}


bool CellularGetSignalQuality(int *rssi)
{
    char response[128];

    if (rssi == NULL)
    {
        return false;
    }

    if (!CellularSendCommand(
            "AT+CSQ\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    /*
     * Ejemplo:
     *
     * +CSQ: 18,99
     * OK
     */

    char *csq = strstr(response, "+CSQ:");

    if (csq == NULL)
    {
        printf("CELLULAR: CSQ response not found\n");
        return false;
    }

    char *colon = strchr(csq, ':');

    if (colon == NULL)
    {
        printf("CELLULAR: Invalid CSQ response\n");
        return false;
    }

    int signal;
    int ber;

    if (sscanf(colon + 1, "%d,%d", &signal, &ber) != 2)
    {
        printf("CELLULAR: Cannot parse CSQ response\n");
        return false;
    }

    *rssi = signal;

    printf(
        "CELLULAR: Signal RSSI = %d, BER = %d\n",
        signal,
        ber
    );

    return true;
}


bool CellularGetOperator(
    char *operator_name,
    size_t operator_size
)
{
    char response[256];

    if (operator_name == NULL || operator_size == 0)
    {
        return false;
    }

    operator_name[0] = '\0';

    if (!CellularSendCommand(
            "AT+COPS?\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    /*
     * Ejemplo:
     *
     * +COPS: 0,0,"Personal",7
     * OK
     */

    char *cops = strstr(response, "+COPS:");

    if (cops == NULL)
    {
        printf("CELLULAR: COPS response not found\n");
        return false;
    }

    char *first_quote = strchr(cops, '"');

    if (first_quote == NULL)
    {
        printf("CELLULAR: Operator name not found\n");
        return false;
    }

    first_quote++;

    char *second_quote = strchr(first_quote, '"');

    if (second_quote == NULL)
    {
        printf("CELLULAR: Invalid COPS response\n");
        return false;
    }

    size_t length = second_quote - first_quote;

    if (length >= operator_size)
    {
        printf("CELLULAR: Operator name buffer too small\n");
        return false;
    }

    memcpy(operator_name, first_quote, length);

    operator_name[length] = '\0';

    printf(
        "CELLULAR: Operator = %s\n",
        operator_name
    );

    return true;
}


bool CellularConfigurePdp(
    const char *apn,
    const char *username,
    const char *password
)
{
    char command[256];
    char response[128];

    if (apn == NULL ||
        username == NULL ||
        password == NULL)
    {
        return false;
    }

    snprintf(
        command,
        sizeof(command),
        "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n",
        apn,
        username,
        password
    );

    if (!CellularSendCommand(
            command,
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        return false;
    }

    if (strstr(response, "OK") == NULL)
    {
        printf("CELLULAR: QICSGP failed\n");
        return false;
    }

    printf("CELLULAR: PDP context configured\n");

    return true;
}



bool CellularActivatePdp(void)
{
    char response[128];

    if (!CellularSendCommand(
            "AT+QIACT=1\r\n",
            response,
            sizeof(response),
            CELLULAR_QIACT_TIMEOUT_MS))
    {
        printf("CELLULAR: QIACT command failed\n");
        return false;
    }

    if (strstr(response, "OK") == NULL)
    {
        printf("CELLULAR: PDP activation failed\n");
        return false;
    }

    printf("CELLULAR: PDP context activated\n");

    return true;
}



bool CellularGetPdpStatus( bool *active, char *ip_address, size_t ip_address_size)
{
    char response[256];

    if (active == NULL ||
        ip_address == NULL ||
        ip_address_size == 0)
    {
        return false;
    }

    *active = false;
    ip_address[0] = '\0';

    if (!CellularSendCommand(
            "AT+QIACT?\r\n",
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        printf("CELLULAR: QIACT? command failed\n");
        return false;
    }

    /*
     * Ejemplo:
     *
     * +QIACT: 1,1,1,"10.123.45.67"
     * OK
     */

    char *qiact = strstr(response, "+QIACT:");

    if (qiact == NULL)
    {
        printf("CELLULAR: QIACT response not found\n");
        return false;
    }

    int context_id;
    int context_state;
    int context_type;

    char ip[64];

    int parsed = sscanf(
        qiact,
        "+QIACT: %d,%d,%d,\"%63[^\"]\"",
        &context_id,
        &context_state,
        &context_type,
        ip
    );

    if (parsed != 4)
    {
        printf("CELLULAR: Cannot parse QIACT response\n");
        return false;
    }

    printf(
        "CELLULAR: PDP ID=%d STATE=%d TYPE=%d IP=%s\n",
        context_id,
        context_state,
        context_type,
        ip
    );

    /*
     * Nos interesa específicamente el contexto 1.
     */
    if (context_id != 1)
    {
        printf("CELLULAR: Unexpected PDP context ID\n");
        return false;
    }

    /*
     * context_state == 1 significa activo.
     */
    if (context_state == 1)
    {
        *active = true;
    }

    /*
     * Copiamos la IP al buffer del usuario.
     */
    if (strlen(ip) >= ip_address_size)
    {
        printf("CELLULAR: IP buffer too small\n");
        return false;
    }

    strcpy(ip_address, ip);

    return true;
}









bool CellularWaitForResponse(
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

    /*
     * Antes que nada, revisamos la cola de URCs: el que buscamos
     * puede haber llegado ya, encolado por CellularEngineStep durante
     * la lectura de OTRO comando (ej: pegado al "SEND OK" de un
     * CellularSendCommand anterior). Sin este chequeo, esta espera
     * bloquearía por gusto hasta el timeout, aunque el dato ya esté
     * disponible desde antes de que empezáramos a esperar.
     */
    if (CellularUrcDequeueMatching(expected, response, response_size))
    {
        return true;
    }

    /*
     * No estaba en cola: bombeamos el engine, que hace lecturas
     * físicas de UART y sigue clasificando y encolando cualquier URC
     * que vaya llegando. Revisamos la cola después de cada lectura.
     */
    uint32_t elapsed_ms = 0;

    while (elapsed_ms < timeout_ms)
    {
        // No nos interesa el canal de "respuesta" acá (no estamos en
        // medio de ningún comando síncrono); solo que los URCs que
        // aparezcan queden encolados.
        int len = CellularEngineStep(CELLULAR_AT_TIMEOUT_MS, NULL, 0);

        if (len > 0)
        {
            if (CellularUrcDequeueMatching(expected, response, response_size))
            {
                return true;
            }
        }

        elapsed_ms += CELLULAR_AT_TIMEOUT_MS;
    }

    printf(
        "CELLULAR: Timeout waiting for: %s\n",
        expected
    );

    return false;
}


/**
 * Consulta el estado de los sockets vía AT+QISTATE? y lo imprime por consola.
 * Devuelve true si pudo hacer la consulta (independientemente de si hay
 * sockets abiertos o no).
 */
bool CellularPrintSocketState(void)
{
    char response[256];

    printf("\n========== QISTATE (sockets activos) ==========\n");

    bool ok = CellularSendCommand(
        "AT+QISTATE?\r\n",
        response,
        sizeof(response),
        5000
    );

    if (!ok)
    {
        printf("No se pudo consultar QISTATE.\n");
        return false;
    }

    if (strstr(response, "+QISTATE:") == NULL)
    {
        printf("No hay sockets abiertos.\n");
    }
    else
    {
        printf("%s\n", response);
    }

    return true;
}


/**
 * Cierra (best-effort) todos los connectID posibles (0..11 en el EG915U-LA).
 * Se ignoran errores individuales: es normal que la mayoría no estén
 * abiertos y el módulo responda ERROR para esos casos.
 */
void CellularCloseAllSockets(void)
{
    char command[32];
    char response[128];

    printf("\n========== Cerrando sockets (barrido QICLOSE) ==========\n");

    for (int id = 0; id <= 11; id++)
    {
        snprintf(command, sizeof(command), "AT+QICLOSE=%d\r\n", id);
        CellularSendCommand(command, response, sizeof(response), 2000);
    }
}




int CellularGetOpenSockets(int *sockets, int max_sockets)
{
    char response[512];

    if (!CellularSendCommand("AT+QISTATE=0\r\n",
                              response,
                              sizeof(response),
                              CELLULAR_AT_TIMEOUT_MS))
    {
        printf("CELLULAR: QISTATE command failed\n");
        return -1;
    }

    // Si el módem respondió ERROR, la consulta en sí falló:
    // no es lo mismo que "0 sockets abiertos".
    if (strstr(response, "ERROR") != NULL)
    {
        printf("CELLULAR: QISTATE returned ERROR: %s\n", response);
        return -1;
    }

    /*
     * Ejemplo de respuesta:
     * +QISTATE: 0,0,"TCP","example.com",80,1,1,0,0,0
     * +QISTATE: 1,0,"UDP","example.com",1234,1,1,0,0,0
     * OK
     */

    int count = 0;
    char *line = strtok(response, "\r\n");
    while (line != NULL && count < max_sockets)
    {
        if (strstr(line, "+QISTATE:") != NULL)
        {
            int id;
            if (sscanf(line, "+QISTATE: %d,", &id) == 1)
            {
                sockets[count++] = id;
            }
        }
        line = strtok(NULL, "\r\n");
    }

    printf("CELLULAR: %d sockets abiertos\n", count);
    return count;
}

// ---------------------------------------------------------------------
// Defines
// ---------------------------------------------------------------------

// Timeout para que el módem devuelva OK + el URC +QIOPEN.
// QIOPEN puede tardar varios segundos en TCP real (resolución DNS +
// handshake), así que va bastante más holgado que un AT command común.
//#define CELLULAR_QIOPEN_TIMEOUT_MS      (10000u)

// Tamaño de buffer de respuesta para QIOPEN: tiene que entrar
// "\r\nOK\r\n+QIOPEN: <id>,<err>\r\n" y algo de margen.
#define CELLULAR_QIOPEN_RESPONSE_SIZE   (256u)

// Longitud máxima del comando AT+QIOPEN formateado
#define CELLULAR_QIOPEN_COMMAND_SIZE    (256u)

// Código de éxito del URC +QIOPEN
#define CELLULAR_QIOPEN_ERR_SUCCESS     (0)

// ---------------------------------------------------------------------
// Implementación
// ---------------------------------------------------------------------

bool CellularOpenSocket(
    int socket_id,
    const char *type,   // "TCP" o "UDP"
    const char *host,
    int port
)
{
    char command[CELLULAR_QIOPEN_COMMAND_SIZE];
    char response[CELLULAR_QIOPEN_RESPONSE_SIZE];

    if (type == NULL || host == NULL || port <= 0) {
        printf("CELLULAR: QIOPEN invalid arguments\n");
        return false;
    }

    snprintf(
        command,
        sizeof(command),
        "AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,0,0\r\n",///////////////////////////
        socket_id,
        type,
        host,
        port
    );

    if (!CellularSendCommand(
            command,
            response,
            sizeof(response),
            CELLULAR_QIOPEN_TIMEOUT_MS))
    {
        printf("CELLULAR: QIOPEN command failed (no response)\n");
        return false;
    }

    if (strstr(response, "OK") == NULL) {
        printf("CELLULAR: Socket %d open failed (no OK): %s\n",
               socket_id, response);
        return false;
    }

    // El "OK" solo confirma que el comando fue aceptado.
    // El resultado real de la apertura llega como URC asíncrono:
    //   +QIOPEN: <connectID>,<err>
    // err == 0 significa éxito; cualquier otro valor es un código
    // de error del módem (p.ej. 563 = fallo de conexión TCP).
    char *urc = strstr(response, "+QIOPEN:");
    if (urc == NULL) {
        printf("CELLULAR: Socket %d open failed (no +QIOPEN URC): %s\n",
               socket_id, response);
        return false;
    }

    int connect_id = -1;
    int err = -1;

    if (sscanf(urc, "+QIOPEN: %d,%d", &connect_id, &err) != 2) {
        printf("CELLULAR: Socket %d open failed (URC parse error): %s\n",
               socket_id, urc);
        return false;
    }

    if (connect_id != socket_id) {
        printf("CELLULAR: Socket open mismatch: pedido %d, URC reporta %d\n",
               socket_id, connect_id);
        return false;
    }

    if (err != CELLULAR_QIOPEN_ERR_SUCCESS) {
        printf("CELLULAR: Socket %d open failed, QIOPEN err=%d\n",
               socket_id, err);
        return false;
    }

    printf("CELLULAR: Socket %d opened (%s %s:%d)\n",
           socket_id, type, host, port);

    return true;
}


bool CellularCloseSocket(int socket_id)
{
    char command[64];
    char response[128];

    snprintf(
        command,
        sizeof(command),
        "AT+QICLOSE=%d\r\n",
        socket_id
    );

    if (!CellularSendCommand(
            command,
            response,
            sizeof(response),
            CELLULAR_AT_TIMEOUT_MS))
    {
        printf("CELLULAR: QICLOSE command failed\n");
        return false;
    }

    if (strstr(response, "OK") == NULL)
    {
        printf("CELLULAR: Failed to close socket %d\n", socket_id);
        return false;
    }

    printf("CELLULAR: Socket %d closed\n", socket_id);
    return true;
}



bool CellularSendTcp(
    int socket_id,
    const char *data,
    size_t length
)
{
    char command[64];
    char response[128];

    if (data == NULL || length == 0)
    {
        printf("CELLULAR: Invalid TCP data\n");
        return false;
    }

    snprintf(
        command,
        sizeof(command),
        "AT+QISEND=%d,%d\r\n",
        socket_id,
        (int)length
    );

    printf(
        "CELLULAR: Sending %zu bytes on TCP socket %d\n",
        length,
        socket_id
    );

    /*
     * 1. Solicitar al módem el envío de datos.
     */
    if (!CellularSendCommand(
            command,
            response,
            sizeof(response),
            CELLULAR_TCP_COMMAND_TIMEOUT_MS))
    {
        printf(
            "CELLULAR: QISEND command failed\n"
        );

        return false;
    }

    /*
     * 2. El módem debe responder con el prompt '>'.
     */
    if (strstr(response, ">") == NULL)
    {
        printf(
            "CELLULAR: QISEND prompt not received\n"
        );

        return false;
    }

    /*
     * 3. Enviar los datos propiamente dichos.
     *
     * NOTA: si el otro extremo contesta muy rápido, esta misma lectura
     * (dentro de CellularSendCommand -> CellularEngineStep) puede traer
     * pegado, además de "SEND OK", el URC de recepción. El engine ya lo
     * clasifica y encola en el momento -- no llega mezclado a
     * 'response' ni se pierde. CellularReceiveTcp lo va a encontrar en
     * la cola cuando lo pida, sin importar si llegó antes o después de
     * que empezara a esperarlo.
     */
    if (!CellularSendCommand(
            data,
            response,
            sizeof(response),
            CELLULAR_TCP_COMMAND_TIMEOUT_MS))
    {
        printf(
            "CELLULAR: TCP data transmission failed\n"
        );

        return false;
    }

    /*
     * 4. El módem debe confirmar el envío.
     */
    if (strstr(response, "SEND OK") == NULL)
    {
        printf(
            "CELLULAR: TCP send rejected\n"
        );

        return false;
    }

    printf(
        "CELLULAR: TCP data sent successfully\n"
    );

    return true;
}



bool CellularReceiveTcp(
    int socket_id,
    char *data,
    size_t data_size
)
{
    char response[256];
    char command[64];
    char expected[64];

    if (data == NULL || data_size == 0)
    {
        printf("CELLULAR: Invalid TCP receive buffer\n");
        return false;
    }

    data[0] = '\0';

    /*
     * Esperar la notificación de que llegaron datos.
     */
    snprintf(
        expected,
        sizeof(expected),
        "+QIURC: \"recv\",%d",
        socket_id
    );

    printf(
        "CELLULAR: Waiting for TCP data on socket %d\n",
        socket_id
    );

    if (!CellularWaitForResponse(
            expected,
            response,
            sizeof(response),
            CELLULAR_TCP_COMMAND_TIMEOUT_MS))
    {
        printf(
            "CELLULAR: No TCP receive notification\n"
        );

        return false;
    }

    /*
     * Solicitar los datos disponibles.
     *
     * data_size - 1 deja espacio para '\0'.
     */
    snprintf(
        command,
        sizeof(command),
        "AT+QIRD=%d,%d\r\n",
        socket_id,
        (int)(data_size - 1)
    );

    if (!CellularSendCommand(
            command,
            response,
            sizeof(response),
            CELLULAR_TCP_COMMAND_TIMEOUT_MS))
    {
        printf(
            "CELLULAR: QIRD command failed\n"
        );

        return false;
    }

    /*
     * Buscar:
     *
     * +QIRD: <cantidad>
     */
    char *qird = strstr(response, "+QIRD:");

    if (qird == NULL)
    {
        printf(
            "CELLULAR: Invalid QIRD response\n"
        );

        return false;
    }

    /*
     * Obtener la cantidad de bytes recibidos.
     *
     * Ejemplo:
     *
     * +QIRD: 5
     */
    int received_length = 0;

    if (sscanf(qird, "+QIRD: %d", &received_length) != 1)
    {
        printf(
            "CELLULAR: Could not parse QIRD length\n"
        );

        return false;
    }

    if (received_length <= 0)
    {
        printf(
            "CELLULAR: QIRD returned no data\n"
        );

        return false;
    }

    /*
     * Buscar el comienzo de la línea que contiene
     * los datos TCP.
     */
    char *payload = strchr(qird, '\n');

    if (payload == NULL)
    {
        printf(
            "CELLULAR: QIRD payload not found\n"
        );

        return false;
    }

    payload++;

    /*
     * Verificar que el buffer del usuario sea suficiente.
     */
    if ((size_t)received_length >= data_size)
    {
        printf(
            "CELLULAR: Receive buffer too small "
            "(received=%d buffer=%zu)\n",
            received_length,
            data_size
        );

        return false;
    }

    /*
     * Copiar EXACTAMENTE los bytes indicados por QIRD.
     *
     * No usamos strlen(), porque después de los datos
     * aparecen elementos del protocolo AT como "OK".
     */
    memcpy(
        data,
        payload,
        (size_t)received_length
    );

    data[received_length] = '\0';

    printf(
        "CELLULAR: TCP data received: %d bytes\n",
        received_length
    );

    return true;
}









/////////////////////////////////////





// Apagado lógico recomendado
/*
bool CellularPowerOff(void) {
    UartHalWriteBytes("AT+QPOWD=1\r\n", strlen("AT+QPOWD=1\r\n"));
    char response[64];
    int len = UartHalReadBytes(response, sizeof(response) - 1, 5000);
    if (len > 0) {
        response[len] = '\0';
        return strstr(response, "POWER DOWN") != NULL;
    }
    return false;
}
    */
bool CellularPowerOff(void)
{
    char response[128];

    printf("CELLULAR: Intentando apagado por software (AT+QPOWD=1)...\n");

    /*
     * Método A (preferido): apagado por software.
     * Timeout generoso porque el módulo hace desregistro de red,
     * cierre de sesiones PDP y guardado en flash antes de cortar.
     */
    if (CellularSendCommand(
            "AT+QPOWD=1\r\n",
            response,
            sizeof(response),
            CELLULAR_QPOWD_TIMEOUT_MS))  // ej. 20000
    {
        if (strstr(response, "POWERED DOWN") != NULL)
        {
            printf("CELLULAR: Apagado por software confirmado.\n");
            return true;
        }
    }

    printf("CELLULAR: Apagado por software fallo o sin confirmacion. "
           "Intentando failsafe por hardware...\n");

    /*
     * Método B (failsafe): pulso de PWRKEY >= 3s, según el manual,
     * para el caso de que la UART no responda o el módulo esté colgado.
     */
    GPIOOn(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(CELLULAR_PWRKEY_POWEROFF_PULSE_MS)); // >= 3000 ms
    GPIOOff(QUECTEL_PWRKEY_PIN);

    // Opcional: si tenés acceso al pin STATUS, confirmá acá que bajó a LOW
    // en vez de asumir éxito ciegamente.

    printf("CELLULAR: Pulso de apagado por hardware enviado.\n");
    return true;
}

// Apagado físico (hard) - solo emergencia
bool CellularPowerOffHard(void) {
    GPIOInit(QUECTEL_PWRKEY_PIN, GPIO_OUTPUT);
    GPIOOff(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(2000)); // mantener bajo ~2s
    GPIOOn(QUECTEL_PWRKEY_PIN);
    vTaskDelay(pdMS_TO_TICKS(5000)); // esperar apagado
    return true;
}

#include <string.h>

bool CellularGetImei(char *imei_out, size_t max_len) {
    if (imei_out == NULL || max_len == 0) {
        return false;
    }
    const char *fake_imei = "123456789012345"; // valor fijo
    strncpy(imei_out, fake_imei, max_len - 1);
    imei_out[max_len - 1] = '\0';
    return true; // éxito
}

bool CellularGetNtpTime(char *timestamp_out, size_t max_len) {
    if (timestamp_out == NULL || max_len == 0) {
        return false;
    }
    const char *fake_time = "2026-08-29 14:44:00"; // valor fijo
    strncpy(timestamp_out, fake_time, max_len - 1);
    timestamp_out[max_len - 1] = '\0';
    return true; // éxito
}

bool CellularSmsSend(const char *number, const char *message){
    return true;
}