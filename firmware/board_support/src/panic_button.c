#include "panic_button.h"

#include "gpio_hal.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


/*==================[macros and definitions]===============================*/

#define PANIC_BUTTON_DEBOUNCE_MS    30

/*==================[internal data declaration]============================*/

static TaskHandle_t panic_button_task_handle = NULL;

static volatile bool panic_button_event = false;

static bool panic_button_initialized = false;


/*==================[internal functions declaration]========================*/

static void PanicButtonISR(void *args);

static void PanicButtonTask(void *args);


/*==================[internal functions definition]=========================*/

/**
 * @brief Rutina de atención de interrupción del botón.
 *
 * Se ejecuta en contexto de interrupción.
 * No realiza debounce ni procesamiento complejo.
 */
static void PanicButtonISR(void *args)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    vTaskNotifyGiveFromISR(
        panic_button_task_handle,
        &higher_priority_task_woken
    );

    if (higher_priority_task_woken)
    {
        portYIELD_FROM_ISR();
    }
}


/**
 * @brief Tarea encargada del debounce del botón.
 */
static void PanicButtonTask(void *args)
{
    TickType_t last_event_time = 0;

    while (1)
    {
        /*
         * Esperamos hasta que la ISR nos notifique
         * que ocurrió una interrupción.
         */
        ulTaskNotifyTake(
            pdTRUE,
            portMAX_DELAY
        );

        TickType_t current_time = xTaskGetTickCount();

        /*
         * Debounce temporal.
         */
        if ((current_time - last_event_time) >=
            pdMS_TO_TICKS(PANIC_BUTTON_DEBOUNCE_MS))
        {
            /* Esperamos 10ms a que pase el caos eléctrico del rebote (flancos falsos al soltar) */
            vTaskDelay(pdMS_TO_TICKS(10));

            /* Verificamos si el botón SIGUE físicamente presionado */
            if (PanicButtonIsPressed()) 
            {
                last_event_time = xTaskGetTickCount();

                /*
                 * Se registra un evento para la aplicación.
                 */
                panic_button_event = true;
            }
        }
    }
}


/*==================[external functions definition]=========================*/

bool PanicButtonInit(void)
{
    /* 
     * SIEMPRE limpiamos el estado al inicializar, 
     * para no arrastrar eventos fantasmas de ejecuciones anteriores. 
     */
    panic_button_event = false;
    /*
     * Configuración del GPIO del botón.
     *
     * GPIO23 utiliza el pull-up definido en el HAL.
     *
     * Estado:
     *
     *   botón liberado  -> HIGH
     *   botón presionado -> LOW
     */
    /* Si ya fue inicializado, no crear otra tarea ni registrar otra ISR */
    if (panic_button_initialized)
    {
        return true;
    }
    
    /*
     * 1. Configuramos GPIO23 como entrada.
     */
    GPIOInit(
        GPIO_PANIC_BTN,
        GPIO_INPUT
    );

    /* Estado inicial conocido */
    panic_button_event = false;

    /*
     * 2. Creamos la tarea que procesará
     *    las notificaciones provenientes de la ISR.
     */
    if (xTaskCreate(
            PanicButtonTask,
            "panic_button_task",
            2048,
            NULL,
            5,
            &panic_button_task_handle
        ) != pdPASS)
    {
        return false;
    }

    /*
     * Activamos la interrupción por flanco descendente.
     *
     * El botón utiliza pull-up:
     *
     *       liberado = 1
     *       presionado = 0
     *
     * Por eso detectamos HIGH -> LOW.
     */
    GPIOActivInt(
        GPIO_PANIC_BTN,
        GPIO_INT_FALLING,
        PanicButtonISR,
        NULL
    );

    /* Actualizamos la bandera para evitar inicializaciones múltiples */
    panic_button_initialized = true;

    return true;
}


bool PanicButtonIsPressed(void)
{
    /*
     * El botón es activo en LOW.
     */
    return !GPIORead(GPIO_PANIC_BTN);
}


bool PanicButtonPressedEvent(void)
{
    if (panic_button_event)
    {
        panic_button_event = false;

        return true;
    }

    return false;
}


/*==================[end of file]============================================*/