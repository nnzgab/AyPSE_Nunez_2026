#include <stdio.h>
#include "unity.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "unity_test_runner.h"

void app_main(void)
{ 
    // Ejecutar solo los tests de GPIO
    printf("Running tests with [gpio] tag\n");
    UNITY_BEGIN();
    unity_run_tests_by_tag("[gpio]", false);
    UNITY_END();

    // Ejecutar solo los tests de UART
    printf("Running tests with [uart] tag\n");
    UNITY_BEGIN();
    unity_run_tests_by_tag("[uart]", false);
    UNITY_END();

    // Opcional: pruebas físicas de loopback UART
    printf("Running tests with [loopback] tag\n");
    UNITY_BEGIN();
    unity_run_tests_by_tag("[loopback]", false);
    UNITY_END();

    // Menú interactivo (para elegir etiquetas en tiempo real)
    ESP_ERROR_CHECK(esp_task_wdt_deinit());
    printf("\n[0_test_runner] Unity menu ready.\n");
    printf("[0_test_runner] Enter '*' to run all tests or [tag] to filter.\n\n");
    unity_run_menu();
}
