#include <stdio.h>

#include "unity.h"
#include "esp_task_wdt.h"
#include "unity_test_runner.h"


void app_main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("       GPIO HAL UNIT TESTS\n");
    printf("========================================\n");

    UNITY_BEGIN();

    unity_run_tests_by_tag("[gpio]", false);

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
    printf("Enter a tag to filter tests\n\n");

    unity_run_menu();
}