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

    /* Ejecuta primero todos los tests con la etiqueta [gpio] */
    //unity_run_tests_by_tag("[gpio]", false);

    /* Agregas esta línea para ejecutar los de UART automáticamente */
    //unity_run_tests_by_tag("[uart]", false);

    /* Agregas esta línea para ejecutar los de BSP led automáticamente */
    //unity_run_tests_by_tag("[led]", false);

    /* Agregas esta línea para ejecutar los de BSP led automáticamente */
    //unity_run_tests_by_tag("[button]", false);

    /* Agregas esta línea para ejecutar los de pwrky automáticamente */
    //unity_run_tests_by_tag("[powerkey]", false);


 
    /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[ready]", false);

      /* Agregas esta línea para ejecutar init cellular automáticamente */
    unity_run_tests_by_tag("[at]", false);
  


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
    printf("Enter a tag (ej: [uart] o [gpio]) to filter tests\n\n");

    unity_run_menu();

}
    