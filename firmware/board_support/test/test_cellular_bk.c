#include "unity.h"
#include "cellular_modem.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
/*
void setUp(void)
{
    // Inicialización común para mantener los tests independientes pero limpios
    CellularModemInit();
}
*/

/*
TEST_CASE("TEST-BSP-MODEM-01 CellularModemInit initializes correctly", "[bsp][cellular]")
{
    bool result = CellularModemInit();
    TEST_ASSERT_TRUE_MESSAGE(result, "CellularModemInit() fallo al configurar UART o GPIO de PWRKEY");
}

TEST_CASE("TEST-BSP-MODEM-02 CellularModem Write and Read raw communication", "[bsp][cellular]")
{
    const char *cmd = "AT\r\n";
    int written = CellularModemWriteRaw((const uint8_t *)cmd, strlen(cmd));
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, written, "Error al escribir bytes crudos hacia el modem");

    uint8_t buffer[64] = {0};
    int read_bytes = CellularModemReadRaw(buffer, sizeof(buffer) - 1, 500);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, read_bytes);
    
    if (read_bytes > 0) {
        printf("--> Bytes detectados en UART: %s (Hex:", buffer);
        for(int i=0; i<read_bytes; i++) printf(" %02X", buffer[i]);
        printf(")\n");
    } else {
        printf("--> [INFO] Sin respuesta en UART (esperado si el modulo esta apagado).\n");
    }
}

TEST_CASE("TEST-BSP-MODEM-03 Interactive Power Pulse verification", "[bsp][cellular][interactive]")
{
    printf("\n[1/2] Preparandose para enviar pulso de encendido al modulo Quectel...\n");
    printf("Observe los indicadores LED de la placa de comunicaciones.\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    CellularModemPowerPulse();
    printf("--> Pulso de encendido enviado (PWRKEY alternado).\n");

    printf("\n[2/2] Esperando 3 segundos para que el modulo bootee y responda 'AT'...\n");
    vTaskDelay(pdMS_TO_TICKS(3000));

    const char *cmd = "AT\r\n";
    CellularModemWriteRaw((const uint8_t *)cmd, strlen(cmd));
    
    uint8_t buffer[64] = {0};
    int read_bytes = CellularModemReadRaw(buffer, sizeof(buffer) - 1, 1000);
    
    // Exigimos que realmente haya leído algo
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, read_bytes, "Fallo: No se leyo ningun byte. ¿Modulo apagado o sin fuente?");
    
    printf("--> Respuesta recibida del modem: %s\n", buffer);

    // Validación estricta: El módem físico encendido DEBE responder con "OK"
    char *response_ok = strstr((char *)buffer, "OK");
    TEST_ASSERT_NOT_NULL_MESSAGE(response_ok, "Fallo critico: Se recibio ruido/basura por UART pero no el 'OK' del modem.");
    
    printf("--> ¡Exito rotundo! El modem respondio correctamente.\n");
}

TEST_CASE("TEST-BSP-MODEM-04 CellularModemHardPowerOff verification", "[bsp][cellular][interactive]")
{
    printf("\n[ATENCION] Iniciando secuencia de apagado duro...\n");
    
    // Ejecutamos el cambio de pin
    //CellularModemHardPowerOff();
    
    // Ahora sí, la cuenta regresiva visible para el usuario en la consola
    for (int i = 5; i > 0; i--) {
        printf("Esperando estabilizacion post apagado... %d segundos restantes\n", i);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    CellularModemHardPowerOff();

    printf("\nVerificando que el modem haya quedado mudo en el UART...\n");
    const char *cmd = "AT\r\n";
    CellularModemWriteRaw((const uint8_t *)cmd, strlen(cmd));
    
    uint8_t buffer[64] = {0};
    int read_bytes = CellularModemReadRaw(buffer, sizeof(buffer) - 1, 1000);
    
    if (read_bytes <= 0 || strstr((char *)buffer, "OK") == NULL) {
        printf("--> ¡Exito! El modem esta completamente apagado (Hard Off verificado).\n");
        TEST_PASS();
    } else {
        printf("--> [FALLO] El modem sigue respondiendo: %s\n", buffer);
        TEST_FAIL_MESSAGE("El apagado duro no surtio efecto en el hardware.");
    }
}

*/





static void send_and_print_command(
    const char *command,
    uint32_t timeout_ms
)
{
    char rx_buffer[512];
    int received;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TX: %s", command);
    printf("----------------------------------------\n");

    CellularModemWrite(
        (const uint8_t *)command,
        strlen(command)
    );

    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    received = CellularModemRead(
        (uint8_t *)rx_buffer,
        sizeof(rx_buffer) - 1U,
        timeout_ms
    );

    if (received > 0)
    {
        rx_buffer[received] = '\0';

        printf(
            "RX:\n%s\n",
            rx_buffer
        );
    }
    else
    {
        printf(
            "RX: SIN RESPUESTA\n"
        );
    }
}

TEST_CASE(
    "TEST-MW-CELLULAR-prueba de fuego",
    "[prueba_fuego]"
)
{
    printf("\n");
    printf("========================================\n");
    printf(" TEST-MW-CELLULAR-PRUEBA DE FUEGO\n");
    printf(" Diagnostico completo del modem\n");
    printf("========================================\n");

    /*
     * Inicializar BSP del modem
     */
    printf("\n--> Inicializando modem...\n");

    CellularModemInit();

    /*
     * Encender modem
     */
    printf("\n--> Encendiendo modem...\n");

    CellularModemPowerPulse();

    /*
     * Esperar RDY
     */
    printf("\n--> Esperando RDY...\n");

    char rx_buffer[512];

    memset(
        rx_buffer,
        0,
        sizeof(rx_buffer)
    );

    int received = CellularModemRead(
        (uint8_t *)rx_buffer,
        sizeof(rx_buffer) - 1U,
        10000U
    );

    if (received > 0)
    {
        rx_buffer[received] = '\0';

        printf(
            "\n[MODEM BOOT]\n%s\n",
            rx_buffer
        );
    }

    /*
     * Esperar un poco después de RDY
     */
    vTaskDelay(
        pdMS_TO_TICKS(2000)
    );

    /*
     * Comunicación básica
     */
    printf("\n");
    printf("========================================\n");
    printf(" COMUNICACION BASICA\n");
    printf("========================================\n");

    send_and_print_command(
        "AT\r\n",
        5000
    );

    send_and_print_command(
        "ATE0\r\n",
        3000U
    );

    /*
     * SIM
     */
    printf("\n");
    printf("========================================\n");
    printf(" PASO 1 - SIM\n");
    printf("========================================\n");

    send_and_print_command(
        "AT+CPIN?\r\n",
        3000U
    );

    /*
     * Señal
     */
    printf("\n");
    printf("========================================\n");
    printf(" PASO 2 - SEÑAL\n");
    printf("========================================\n");

    send_and_print_command(
        "AT+CSQ\r\n",
        3000U
    );

    /*
     * Registro
     */
    printf("\n");
    printf("========================================\n");
    printf(" PASO 3 - REGISTRO\n");
    printf("========================================\n");

    send_and_print_command(
        "AT+CREG?\r\n",
        3000U
    );

    send_and_print_command(
        "AT+CGREG?\r\n",
        3000U
    );

    send_and_print_command(
        "AT+COPS?\r\n",
        5000U
    );

    /*
     * Reinicio de radio
     */
    printf("\n");
    printf("========================================\n");
    printf(" PASO 4 - REINICIO DE RADIO\n");
    printf("========================================\n");

    send_and_print_command(
        "AT+CFUN=0\r\n",
        5000U
    );

    printf(
        "--> Esperando 3 segundos...\n"
    );

    vTaskDelay(
        pdMS_TO_TICKS(3000U)
    );

    send_and_print_command(
        "AT+CFUN=1\r\n",
        10000U
    );

    printf(
        "--> Esperando 10 segundos...\n"
    );

    vTaskDelay(
        pdMS_TO_TICKS(10000U)
    );

    /*
     * Registro después de CFUN
     */
    printf("\n");
    printf("========================================\n");
    printf(" REGISTRO DESPUES DE CFUN\n");
    printf("========================================\n");

    send_and_print_command(
        "AT+CREG?\r\n",
        3000U
    );

    send_and_print_command(
        "AT+CGREG?\r\n",
        3000U
    );

    send_and_print_command(
        "AT+COPS?\r\n",
        5000U
    );

    /*
     * PDP
     */
    printf("\n");
    printf("========================================\n");
    printf(" PASO 5 - CONFIGURAR PDP\n");
    printf("========================================\n");

    send_and_print_command(
        "AT+QICSGP=1,1,\"datos.personal.com\",\"datos\",\"datos\",1\r\n",
        5000U
    );

    /*
     * Activar PDP
     */
    printf("\n");
    printf("========================================\n");
    printf(" PASO 6 - ACTIVAR PDP\n");
    printf("========================================\n");

    send_and_print_command(
        "AT+QIACT=1\r\n",
        150000U
    );

    /*
     * Consultar PDP / IP
     */
    printf("\n");
    printf("========================================\n");
    printf(" PASO 7 - CONSULTAR IP\n");
    printf("========================================\n");

    send_and_print_command(
        "AT+QIACT?\r\n",
        5000U
    );

    /*
     * Fin
     */
    printf("\n");
    printf("========================================\n");
    printf(" FIN DE PRUEBA DE FUEGO\n");
    printf("========================================\n");

    TEST_PASS();
}