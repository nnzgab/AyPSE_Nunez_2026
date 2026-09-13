
#include "unity.h"

#include "panic_handler.h"
#include "status_indicator.h"

#include "gpio_hal.h"
#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>


/*==================[tests]==================================================*/


TEST_CASE(
    "PanicHandlerInit starts in NORMAL state",
    "[panic_handler]"
)
{
    TEST_ASSERT_TRUE_MESSAGE(
        PanicHandlerInit(),
        "PanicHandlerInit() fallo"
    );

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_FALSE(
        PanicHandlerIsActive()
    );
}


TEST_CASE(
    "PanicHandler remains NORMAL without button press",
    "[panic_handler]"
)
{
    TEST_ASSERT_TRUE(
        PanicHandlerInit()
    );

    /*
     * Ejecutamos varios ciclos del middleware.
     *
     * No se presiona el botón durante esta prueba.
     */
    for (int i = 0; i < 10; i++)
    {
        PanicHandlerRunStep();

        vTaskDelay(
            pdMS_TO_TICKS(25)
        );
    }

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_FALSE(
        PanicHandlerIsActive()
    );
}


TEST_CASE(
    "PanicHandler detects panic button",
    "[panic_handler][button]"
)
{
    TEST_ASSERT_TRUE(
        PanicHandlerInit()
    );

    printf("\n");
    printf("========================================\n");
    printf(" PANIC HANDLER - BUTTON TEST\n");
    printf("========================================\n");

    printf("\n");
    printf("Presione el boton de PANICO una vez.\n");
    printf("Tiene 10 segundos.\n\n");

    bool panic_detected = false;

    /*
     * Esperamos hasta 10 segundos buscando
     * que PanicHandler cambie a PENDING.
     */
    for (int i = 0; i < 400; i++)
    {
        PanicHandlerRunStep();

        if (PanicHandlerIsActive())
        {
            panic_detected = true;
            break;
        }

        vTaskDelay(
            pdMS_TO_TICKS(25)
        );
    }

    TEST_ASSERT_TRUE_MESSAGE(
        panic_detected,
        "No se detecto la alarma de panico"
    );

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_PENDING,
        PanicHandlerGetStatus()
    );

    printf("\n");
    printf("ALARMA DETECTADA CORRECTAMENTE\n");
    printf("Estado = PANIC_STATUS_PENDING\n");
}


TEST_CASE(
    "PanicHandlerClear returns to NORMAL",
    "[panic_handler]"
)
{
    TEST_ASSERT_TRUE(
        PanicHandlerInit()
    );

    /*
     * Clear debe garantizar que el estado
     * vuelva a NORMAL.
     */
    PanicHandlerClear();

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_FALSE(
        PanicHandlerIsActive()
    );
}


/*==================[visual demonstration]==================================*/

TEST_CASE(
    "PanicHandler visual demonstration",
    "[panic_handler][demo]"
)
{
    TEST_ASSERT_TRUE(
        PanicHandlerInit()
    );

    printf("\n");
    printf("========================================\n");
    printf(" DEMOSTRACION PANIC HANDLER\n");
    printf("========================================\n");

    printf("\n");
    printf("[1] SISTEMA NORMAL\n");
    printf("    Estado = NORMAL\n");

    vTaskDelay(
        pdMS_TO_TICKS(5000)
    );


    printf("\n");
    printf("[2] ESPERANDO PULSACION\n");
    printf("    Presione el boton de panico.\n");

    bool panic_detected = false;

    /*
     * Esperamos hasta 20 segundos.
     */
    for (int i = 0; i < 800; i++)
    {
        PanicHandlerRunStep();

        if (PanicHandlerIsActive())
        {
            panic_detected = true;
            break;
        }

        vTaskDelay(
            pdMS_TO_TICKS(25)
        );
    }

    TEST_ASSERT_TRUE_MESSAGE(
        panic_detected,
        "No se detecto la pulsacion del boton"
    );


    printf("\n");
    printf("[3] ALARMA DETECTADA\n");
    printf("    Estado = PENDING\n");

    vTaskDelay(
        pdMS_TO_TICKS(5000)
    );


    printf("\n");
    printf("[4] TRANSMISION SIMULADA\n");
    printf("    La alarma permanece pendiente.\n");

    vTaskDelay(
        pdMS_TO_TICKS(5000)
    );


    printf("\n");
    printf("[5] SERVIDOR CONFIRMA RECEPCION\n");

    PanicHandlerClear();

    printf("    Estado = NORMAL\n");

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    vTaskDelay(
        pdMS_TO_TICKS(5000)
    );


    printf("\n");
    printf("========================================\n");
    printf(" FIN DE DEMOSTRACION\n");
    printf("========================================\n");
}

//////////////////////////
/*==================[macros]================================================*/

#define COMBINED_WAIT_STEP_MS       25
#define COMBINED_BUTTON_TIMEOUT_MS  10000
#define COMBINED_STATE_TIME_MS      5000


/*==================[test]===================================================*/

TEST_CASE(
    "Combined panic and status indicator test",
    "[combined-test]"
)
{
    /*----------------------------------------------------------
     * Inicialización
     *----------------------------------------------------------*/

    TEST_ASSERT_TRUE_MESSAGE(
        StatusIndicatorInit(),
        "StatusIndicatorInit() fallo"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        PanicHandlerInit(),
        "PanicHandlerInit() fallo"
    );


    printf("\n");
    printf("========================================\n");
    printf("       COMBINED PANIC TEST\n");
    printf("========================================\n");


    /*----------------------------------------------------------
     * 1. SISTEMA NORMAL
     *----------------------------------------------------------*/

    printf("\n");
    printf("[1] SISTEMA NORMAL\n");
    printf("    GPIO4 = OFF\n");
    printf("    Estado = NORMAL\n");

    StatusIndicatorSetPanic(false);

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_EQUAL(
        0,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );

    vTaskDelay(
        pdMS_TO_TICKS(COMBINED_STATE_TIME_MS)
    );


    /*----------------------------------------------------------
     * 2. ESPERANDO PULSACION
     *----------------------------------------------------------*/

    printf("\n");
    printf("[2] ESPERANDO PULSACION\n");
    printf("    Presione el boton de PANICO.\n");

    bool panic_detected = false;

    int elapsed_ms = 0;

    while (elapsed_ms < COMBINED_BUTTON_TIMEOUT_MS)
    {
        /*
         * Procesamos el middleware.
         */
        PanicHandlerRunStep();

        /*
         * Verificamos si la pulsación produjo
         * una alarma.
         */
        if (PanicHandlerIsActive())
        {
            panic_detected = true;
            break;
        }

        vTaskDelay(
            pdMS_TO_TICKS(COMBINED_WAIT_STEP_MS)
        );

        elapsed_ms += COMBINED_WAIT_STEP_MS;
    }


    TEST_ASSERT_TRUE_MESSAGE(
        panic_detected,
        "No se detecto la pulsacion del boton"
    );


    /*----------------------------------------------------------
     * 3. ALARMA DETECTADA
     *----------------------------------------------------------*/

    printf("\n");
    printf("[3] ALARMA DE PANICO DETECTADA\n");
    printf("    Estado = PENDING\n");
    printf("    GPIO4 = ON\n");

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_PENDING,
        PanicHandlerGetStatus()
    );


    /*
     * La aplicación representa el estado
     * de alarma mediante el indicador.
     */
    StatusIndicatorSetPanic(true);

    TEST_ASSERT_EQUAL(
        1,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );


    vTaskDelay(
        pdMS_TO_TICKS(COMBINED_STATE_TIME_MS)
    );


    /*----------------------------------------------------------
     * 4. TRANSMISION SIMULADA
     *----------------------------------------------------------*/

    printf("\n");
    printf("[4] TRANSMITIENDO ALARMA\n");
    printf("    GPIO4 = ON\n");
    printf("    Transmision simulada...\n");

    /*
     * En esta etapa no utilizamos Cellular.
     *
     * Solamente mantenemos la alarma activa.
     */
    TEST_ASSERT_TRUE(
        PanicHandlerIsActive()
    );

    TEST_ASSERT_EQUAL(
        1,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );


    vTaskDelay(
        pdMS_TO_TICKS(COMBINED_STATE_TIME_MS)
    );


    /*----------------------------------------------------------
     * 5. SERVIDOR CONFIRMA
     *----------------------------------------------------------*/

    printf("\n");
    printf("[5] SERVIDOR CONFIRMA RECEPCION\n");

    /*
     * Simulamos la confirmación del servidor.
     */
    PanicHandlerClear();

    /*
     * La aplicación actualiza el indicador.
     */
    StatusIndicatorSetPanic(false);


    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_FALSE(
        PanicHandlerIsActive()
    );

    TEST_ASSERT_EQUAL(
        0,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );


    printf("    Estado = NORMAL\n");
    printf("    GPIO4 = OFF\n");


    vTaskDelay(
        pdMS_TO_TICKS(COMBINED_STATE_TIME_MS)
    );


    /*----------------------------------------------------------
     * 6. FIN
     *----------------------------------------------------------*/

    printf("\n");
    printf("========================================\n");
    printf("       FIN COMBINED PANIC TEST\n");
    printf("========================================\n");
}


/////////////////******************************///////////////////
/*
#include "unity.h"

#include "panic_handler.h"
#include "status_indicator.h"

#include "gpio_hal.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
*/

/*==================[macros]================================================*/

#define COMBINED_WAIT_STEP_MS       25
#define COMBINED_BUTTON_TIMEOUT_MS  10000
#define COMBINED_STATE_TIME_MS      5000


/*==================[test]===================================================*/

TEST_CASE(
    "Combined panic and status indicator test",
    "[combined-test_]"
)
{
    /*----------------------------------------------------------
     * Inicialización
     *----------------------------------------------------------*/

    TEST_ASSERT_TRUE_MESSAGE(
        StatusIndicatorInit(),
        "StatusIndicatorInit() fallo"
    );

    TEST_ASSERT_TRUE_MESSAGE(
        PanicHandlerInit(),
        "PanicHandlerInit() fallo"
    );


    printf("\n");
    printf("========================================\n");
    printf("       COMBINED PANIC TEST\n");
    printf("========================================\n");


    printf("\n");
    printf("========================================\n");
    printf("       COMBINED PANIC TEST\n");
    printf("========================================\n");


    /*----------------------------------------------------------
     * 1. MODULO APAGADO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[1] MODULO APAGADO\n");
    printf("    GPIO4 = OFF\n");
    printf("    GPIO5 = OFF\n");

    StatusIndicatorSetPanic(false);

    StatusIndicatorSetCellular(
        CELLULAR_STATUS_OFF
    );

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_EQUAL(
        CELLULAR_STATUS_OFF,
        StatusIndicatorGetCellular()
    );

    TEST_ASSERT_EQUAL(
        0,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );

    TEST_ASSERT_EQUAL(
        0,
        GPIORead(GPIO_QUECTEL_LED_STATUS)
    );

    vTaskDelay(
        pdMS_TO_TICKS(COMBINED_STATE_TIME_MS)
    );


    /*----------------------------------------------------------
     * 2. MODULO ARRANCANDO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[2] MODULO ARRANCANDO\n");
    printf("    GPIO4 = OFF\n");
    printf("    GPIO5 = ON FIJO\n");

    StatusIndicatorSetCellular(
        CELLULAR_STATUS_STARTING
    );

    TEST_ASSERT_EQUAL(
        CELLULAR_STATUS_STARTING,
        StatusIndicatorGetCellular()
    );

    TEST_ASSERT_EQUAL(
        0,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );

    TEST_ASSERT_EQUAL(
        1,
        GPIORead(GPIO_QUECTEL_LED_STATUS)
    );

    vTaskDelay(
        pdMS_TO_TICKS(COMBINED_STATE_TIME_MS)
    );


    /*----------------------------------------------------------
     * 3. BUSCANDO RED
     *----------------------------------------------------------*/

    printf("\n");
    printf("[3] BUSCANDO RED\n");
    printf("    GPIO4 = OFF\n");
    printf("    GPIO5 = 200 ms ON / 1800 ms OFF\n");

    StatusIndicatorSetCellular(
        CELLULAR_STATUS_SEARCHING
    );

    TEST_ASSERT_EQUAL(
        CELLULAR_STATUS_SEARCHING,
        StatusIndicatorGetCellular()
    );

    TEST_ASSERT_EQUAL(
        0,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );

    /*
     * Ejecutamos el patrón durante aproximadamente
     * 5 segundos.
     */
    for (int i = 0; i < 200; i++)
    {
        StatusIndicatorRunStep();

        vTaskDelay(
            pdMS_TO_TICKS(COMBINED_WAIT_STEP_MS)
        );
    }


    /*----------------------------------------------------------
     * 4. RED REGISTRADA - SISTEMA LISTO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[4] RED REGISTRADA - SISTEMA LISTO\n");
    printf("    GPIO4 = OFF\n");
    printf("    GPIO5 = 1800 ms ON / 200 ms OFF\n");

    StatusIndicatorSetCellular(
        CELLULAR_STATUS_READY
    );

    TEST_ASSERT_EQUAL(
        CELLULAR_STATUS_READY,
        StatusIndicatorGetCellular()
    );

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_EQUAL(
        0,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );

    /*
     * El sistema queda disponible para recibir
     * una pulsación.
     */
    for (int i = 0; i < 200; i++)
    {
        StatusIndicatorRunStep();

        vTaskDelay(
            pdMS_TO_TICKS(COMBINED_WAIT_STEP_MS)
        );
    }


    /*----------------------------------------------------------
     * 5. ESPERANDO PULSACION
     *----------------------------------------------------------*/

    printf("\n");
    printf("[5] ESPERANDO PULSACION\n");
    printf("    Presione el boton de PANICO.\n");
    printf("    GPIO5 = READY\n");

    bool panic_detected = false;

    int elapsed_ms = 0;

    while (elapsed_ms < COMBINED_BUTTON_TIMEOUT_MS)
    {
        /*
         * Procesamos el middleware del botón.
         */
        PanicHandlerRunStep();

        /*
         * Procesamos el indicador visual.
         */
        StatusIndicatorRunStep();

        /*
         * Verificamos si se generó la alarma.
         */
        if (PanicHandlerIsActive())
        {
            panic_detected = true;
            break;
        }

        vTaskDelay(
            pdMS_TO_TICKS(COMBINED_WAIT_STEP_MS)
        );

        elapsed_ms += COMBINED_WAIT_STEP_MS;
    }

    TEST_ASSERT_TRUE_MESSAGE(
        panic_detected,
        "No se detecto la pulsacion del boton"
    );


    /*----------------------------------------------------------
     * 6. ALARMA DE PANICO
     *----------------------------------------------------------*/

    printf("\n");
    printf("[6] ALARMA DE PANICO DETECTADA\n");
    printf("    Estado = PENDING\n");
    printf("    GPIO4 = ON\n");
    printf("    GPIO5 = READY\n");

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_PENDING,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_TRUE(
        PanicHandlerIsActive()
    );

    /*
     * Activamos el indicador de alarma.
     */
    StatusIndicatorSetPanic(true);

    TEST_ASSERT_EQUAL(
        1,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );

    TEST_ASSERT_EQUAL(
        CELLULAR_STATUS_READY,
        StatusIndicatorGetCellular()
    );

    /*
     * La alarma permanece activa mientras
     * el sistema espera para transmitir.
     */
    for (int i = 0; i < 200; i++)
    {
        StatusIndicatorRunStep();

        vTaskDelay(
            pdMS_TO_TICKS(COMBINED_WAIT_STEP_MS)
        );
    }


    /*----------------------------------------------------------
     * 7. TRANSMITIENDO ALARMA
     *----------------------------------------------------------*/

    printf("\n");
    printf("[7] TRANSMITIENDO ALARMA\n");
    printf("    GPIO4 = ON\n");
    printf("    GPIO5 = FAST BLINK\n");
    printf("    125 ms ON / 125 ms OFF\n");

    /*
     * Simulamos que la aplicación comienza
     * la transmisión de la alarma.
     */
    StatusIndicatorSetCellular(
        CELLULAR_STATUS_TRANSMITTING
    );

    TEST_ASSERT_EQUAL(
        CELLULAR_STATUS_TRANSMITTING,
        StatusIndicatorGetCellular()
    );

    TEST_ASSERT_TRUE(
        PanicHandlerIsActive()
    );

    TEST_ASSERT_EQUAL(
        1,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );

    /*
     * Ejecutamos el patrón de transmisión
     * durante aproximadamente 5 segundos.
     */
    for (int i = 0; i < 200; i++)
    {
        StatusIndicatorRunStep();

        vTaskDelay(
            pdMS_TO_TICKS(COMBINED_WAIT_STEP_MS)
        );
    }


    /*----------------------------------------------------------
     * 8. SERVIDOR CONFIRMA RECEPCION
     *----------------------------------------------------------*/

    printf("\n");
    printf("[8] SERVIDOR CONFIRMA RECEPCION\n");

    /*
     * Simulamos la confirmación del servidor.
     */
    PanicHandlerClear();

    /*
     * La alarma deja de estar activa.
     */
    StatusIndicatorSetPanic(false);

    /*
     * La transmisión finalizó.
     * El módulo vuelve a estar disponible.
     */
    StatusIndicatorSetCellular(
        CELLULAR_STATUS_READY
    );

    TEST_ASSERT_EQUAL(
        PANIC_STATUS_NORMAL,
        PanicHandlerGetStatus()
    );

    TEST_ASSERT_FALSE(
        PanicHandlerIsActive()
    );

    TEST_ASSERT_EQUAL(
        CELLULAR_STATUS_READY,
        StatusIndicatorGetCellular()
    );

    TEST_ASSERT_EQUAL(
        0,
        GPIORead(GPIO_PANIC_LED_STATUS)
    );

    printf("    Estado = NORMAL\n");
    printf("    GPIO4 = OFF\n");
    printf("    GPIO5 = READY\n");


    /*----------------------------------------------------------
     * 9. SISTEMA LISTO NUEVAMENTE
     *----------------------------------------------------------*/

    printf("\n");
    printf("[9] SISTEMA LISTO NUEVAMENTE\n");
    printf("    Esperando una nueva alarma...\n");
    printf("    GPIO4 = OFF\n");
    printf("    GPIO5 = READY\n");

    /*
     * Dejamos funcionando nuevamente
     * el indicador READY.
     */
    for (int i = 0; i < 200; i++)
    {
        StatusIndicatorRunStep();

        vTaskDelay(
            pdMS_TO_TICKS(COMBINED_WAIT_STEP_MS)
        );
    }


    /*----------------------------------------------------------
     * 10. FIN
     *----------------------------------------------------------*/

    printf("\n");
    printf("========================================\n");
    printf("       FIN COMBINED PANIC TEST\n");
    printf("========================================\n");
}