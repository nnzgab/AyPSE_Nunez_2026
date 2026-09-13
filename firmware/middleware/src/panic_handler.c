#include "panic_handler.h"
#include "panic_button.h"   /* Capa BSP: PanicButtonInit, PanicButtonIsPressed, PanicButtonAttachInterrupt */
#include "board_clock.h"    /* Capa BSP: BoardClockGetMs */
#include <stddef.h>

#define DEBOUNCE_TIME_MS      50U

static panic_event_cb_t s_panic_cb = NULL;
static uint16_t          s_sequence_number = 1U;

/* Estado interno de la máquina de detección no bloqueante */
static volatile bool     s_isr_flag = false;
static uint32_t          s_isr_timestamp_ms = 0U;
static bool              s_debounce_pending = false;

/* ============================================================================
 * ISR interna del pulsador (Callback registrado en el BSP)
 * ============================================================================ */
static void PanicHandler_OnButtonISR(void *arg) {
    (void)arg;
    if (!s_debounce_pending) {
        s_isr_flag = true;
    }
}

/* ============================================================================
 * Inicialización del Módulo (100% C puro, agnóstico de RTOS)
 * ============================================================================ */
panic_handler_err_t PanicHandler_Init(panic_event_cb_t callback) {
    if (callback == NULL) {
        return PANIC_HANDLER_ERR_PARAM;
    }

    s_panic_cb = callback;
    s_sequence_number = 1U;
    s_isr_flag = false;
    s_debounce_pending = false;
    s_isr_timestamp_ms = 0U;

    if (!PanicButtonInit()) {
        return PANIC_HANDLER_ERR_INIT;
    }

    /* Conecta la interrupción física al handler interno */
    PanicButtonAttachInterrupt(PanicHandler_OnButtonISR, NULL);

    return PANIC_HANDLER_OK;
}

/* ============================================================================
 * Función de Pasada No Bloqueante (RunStep - Estándar C Puro)
 * ============================================================================ */
void PanicHandler_RunStep(void) {
    uint32_t now = BoardClockGetMs();

    // 1. Detectar si la ISR levantó la bandera de pulsación
    if (s_isr_flag) {
        s_isr_flag = false;
        s_debounce_pending = true;
        s_isr_timestamp_ms = now;
    }

    // 2. Procesar el tiempo de debounce de forma no bloqueante
    if (s_debounce_pending) {
        if ((now - s_isr_timestamp_ms) >= DEBOUNCE_TIME_MS) {
            s_debounce_pending = false;

            // Reconfirmación de presión física
            if (PanicButtonIsPressed()) {
                uint16_t seq = s_sequence_number++;
                if (s_panic_cb != NULL) {
                    s_panic_cb(seq);
                }
            }
        }
    }
}
