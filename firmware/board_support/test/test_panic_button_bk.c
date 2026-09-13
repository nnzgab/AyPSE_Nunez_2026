#include "unity.h"

#include "panic_button.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


TEST_CASE("TEST-BSP-BUTTON-03 Interactive press and release verification", "[bsp][button][interactive]")
{
    PanicButtonInit();
    
    // Paso 1: Validar que arranquemos con el botón suelto (estado de reposo)
    printf("\n[1/3] Por favor, ASEGURESE DE SOLTAR el boton de panico...\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Si ya arranca en true, significa que o está presionado o el pin está invertido / flotando
    TEST_ASSERT_FALSE_MESSAGE(PanicButtonIsPressed(), "Error: El boton parece estar presionado al inicio.");
    printf("--> Estado de reposo (suelto) verificado OK.\n");

    // Paso 2: Pedir la pulsación y validar que el código lo detecte
    printf("\n[2/3] ¡Ahora MANTENGA PRESIONADO el boton de panico (tiene 5 segundos)...\n");
    bool press_detected = false;
    for (int i = 0; i < 50; i++) { // 50 iteraciones * 100ms = 5 segundos
        if (PanicButtonIsPressed()) {
            press_detected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    TEST_ASSERT_TRUE_MESSAGE(press_detected, "Timeout: Nunca se detecto la pulsacion.");
    printf("--> ¡Pulsacion detectada correctamente!\n");

    // Paso 3: Pedir que lo suelte y validar que vuelva a false
    printf("\n[3/3] Ahora SUELTE el boton...\n");
    bool release_detected = false;
    for (int i = 0; i < 50; i++) {
        if (!PanicButtonIsPressed()) { // Vuelve a falso cuando se suelta
            release_detected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    TEST_ASSERT_TRUE_MESSAGE(release_detected, "Timeout: El boton no detecto la liberacion.");
    printf("--> ¡Liberacion detectada correctamente!\n");

    printf("\n[EXITO] ¡Ciclo completo (reposo -> presionado -> liberado) validado con exito!\n");
}