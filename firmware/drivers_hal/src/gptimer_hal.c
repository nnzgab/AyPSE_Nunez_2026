/**
 * @file gptimer_hal.c
 * @author Nuñez Gabriel Eduardo (nunezgabrieleduardo@gmail.com)
 * @brief General Purpose Timer HAL driver implementation.
 * @version 0.1
 * @date 2026-10-02
 * @copyright Copyright (c) 2026
 */

/*==================[inclusions]=============================================*/
#include "gptimer_hal.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include <stdbool.h>

/*==================[macros and definitions]=================================*/
#define GPTIMER_RESOLUTION_HZ   1000000U  /* 1 MHz -> 1 tick = 1 us */
#define US_PER_MS                1000ULL

/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/
static gptimer_handle_t gptimer_handle = NULL;
static bool gptimer_initialized = false;

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/
int8_t GpTimerInit(void) {
    if (gptimer_initialized) {
        return HAL_GPTIMER_OK;
    }

    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = GPTIMER_RESOLUTION_HZ,
    };

    if (gptimer_new_timer(&config, &gptimer_handle) != ESP_OK) {
        return HAL_GPTIMER_ERROR;
    }

    if (gptimer_enable(gptimer_handle) != ESP_OK) {
        return HAL_GPTIMER_ERROR;
    }

    if (gptimer_start(gptimer_handle) != ESP_OK) {
        return HAL_GPTIMER_ERROR;
    }

    gptimer_initialized = true;
    return HAL_GPTIMER_OK;
}

uint32_t GpTimerGetMs(void) {
    uint64_t raw_count = 0;

    if (!gptimer_initialized) {
        return 0;
    }

    gptimer_get_raw_count(gptimer_handle, &raw_count);

    return (uint32_t)(raw_count / US_PER_MS);
}

void GpTimerDelayMs(uint32_t ms) {
    if (!gptimer_initialized) {
        return;
    }

    uint32_t start = GpTimerGetMs();

    while ((GpTimerGetMs() - start) < ms) {
        /* busy-wait, sin ceder CPU al scheduler */
    }
}

/*==================[end of file]============================================*/