#include <stdio.h>

#include "unity.h"
#include "esp_task_wdt.h"
#include "unity_test_runner.h"


void app_main(void)
{
    printf("\n");
    printf("\n========================================\n");
    printf("       EJECUCION AUTOMATICA DE TESTS\n");
    printf("========================================\n");

    UNITY_BEGIN();
    /* 0. Pruebas de drivers HAL (etiqueta [drivers_hal]) */
    //printf("\n>>> BLOQUE 0: PRUEBAS DE DRIVERS HAL <<<\n");
    //unity_run_tests_by_tag("[drivers_hal]", false);

    /* 1. Pruebas del módulo LED (etiqueta [led]) */
    //printf("\n>>> BLOQUE 1: PRUEBAS DE LED <<<\n");
    //unity_run_tests_by_tag("[led]", false);

    /* 2. Pruebas del módulo Pulsador (etiqueta [panic_button]) */
    //printf("\n>>> BLOQUE 2: PRUEBAS DE PULSADOR <<<\n");
    //unity_run_tests_by_tag("[panic_button]", false);

    /* 3. Pruebas del módulo Módem Celular (etiqueta [cellular]) */
    printf("\n>>> BLOQUE 3: PRUEBAS DE MODEM CELULAR <<<\n");
    unity_run_tests_by_tag("[cellular]", false);

    /* 4. Pruebas del módulo Status Indicator (etiqueta [status_indicator]) */
    //printf("\n>>> BLOQUE 4: PRUEBAS DE STATUS INDICATOR <<<\n");
    //unity_run_tests_by_tag("[status_indicator]", false);


    /* 5. Pruebas del módulo Panic Handler (etiqueta [panic_handler]) */
    //printf("\n>>> BLOQUE 5: PRUEBAS DE PANIC HANDLER <<<\n");
    //unity_run_tests_by_tag("[panic_handler]", false);


    /* 6. Pruebas del módulo Event Frame (etiqueta [event_frame]) */
    //printf("\n>>> BLOQUE 6: PRUEBAS DE EVENT FRAME <<<\n"); 
    //unity_run_tests_by_tag("[event_frame]", false);


    /* 7. Pruebas del módulo Cellular Net (etiqueta [cellular_net]) */
    //printf("\n>>> BLOQUE 7: PRUEBAS DE CELLULAR NET <<<\n");
    //unity_run_tests_by_tag("[cellular_net]", false);
    





    UNITY_END();


    /*
     * El menú interactivo permite ejecutar posteriormente
     * otros tests de forma manual.
     */
    ESP_ERROR_CHECK(esp_task_wdt_deinit());

    printf("\n");
    printf("========================================\n");
    printf("       UNITY TEST MENU\n");
    printf("========================================\n");

    printf("Enter '*' to run all tests\n");
    printf("Enter a tag (ej: [uart], [gpio], [led], [panic_button] o [cellular]) to filter tests\n\n");

    unity_run_menu();

}
    