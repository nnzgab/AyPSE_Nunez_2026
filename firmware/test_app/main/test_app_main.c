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
    //unity_run_tests_by_tag("[at]", false);

          /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[echo]", false);

              /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[full]", false);
  
              /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[imsi]", false);
  
    /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[network]", false);

    /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[cereg]", false);

    /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[csq]", false);

    /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[cops]", false);

    /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[pdp]", false);

        /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[act-pdp]", false);

           /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[pdp-status]", false);

          /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[tcp]", false);

              /* Agregas esta línea para ejecutar init cellular automáticamente */
    unity_run_tests_by_tag("[power-off]", false);
  
                 /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[socket]", false);
  

              /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[sendrecv]", false);

                  /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[sendrecv_]", false);

                  /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[send_]", false);

                  /* Agregas esta línea para ejecutar init cellular automáticamente */
    //unity_run_tests_by_tag("[receivetcp_]", false);


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
    