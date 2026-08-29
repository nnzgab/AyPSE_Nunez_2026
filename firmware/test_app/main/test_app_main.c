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

    //unity_run_tests_by_tag("[gpio]", false);

    //unity_run_tests_by_tag("[uart]", false);

    //unity_run_tests_by_tag("[led]", false);

    //unity_run_tests_by_tag("[button]", false);

    /*BSP-cellular*/
///*
    unity_run_tests_by_tag("[poweron]", false);

    unity_run_tests_by_tag("[at]", false);

    unity_run_tests_by_tag("[echo]", false);

    unity_run_tests_by_tag("[full]", false);
  
    unity_run_tests_by_tag("[imsi]", false);
  
    unity_run_tests_by_tag("[network]", false);

    unity_run_tests_by_tag("[cereg]", false);

    unity_run_tests_by_tag("[csq]", false);

    unity_run_tests_by_tag("[cops]", false);

    unity_run_tests_by_tag("[pdp]", false);

    unity_run_tests_by_tag("[act-pdp]", false);

    unity_run_tests_by_tag("[pdp-status]", false);

    unity_run_tests_by_tag("[tcp]", false);
  
    unity_run_tests_by_tag("[socket]", false);
  
    unity_run_tests_by_tag("[sendrecv]", false);

    unity_run_tests_by_tag("[sendrecv_]", false);

    unity_run_tests_by_tag("[send_]", false);

    unity_run_tests_by_tag("[receivetcp_]", false);
//*/
    unity_run_tests_by_tag("[power-off]", false);

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
    