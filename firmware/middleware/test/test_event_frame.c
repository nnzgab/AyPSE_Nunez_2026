#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "event_frame.h"

#define TEST_IMEI   "123456789012345"

/* ============================================================================
 * TEST-MW-FRAME-01: Validación de parámetros NULOS
 * ============================================================================ */
TEST_CASE("TEST-MW-FRAME-01 EventFrame null parameter validation", "[middleware][event_frame]") {
    printf("\n========================================\n");
    printf(" TEST-MW-FRAME-01 VALIDACION DE PARAMETROS NULOS\n");
    printf("========================================\n");

    event_data_t event = {
        .event_type = EVENT_TYPE_PANIC_ALERT,
        .sequence_number = 1
    };
    strncpy(event.imei, TEST_IMEI, sizeof(event.imei));

    char text_buf[64];
    uint8_t bin_buf[32];
    uint16_t packed_len = 0;

    // 1. Text NULL event
    TEST_ASSERT_EQUAL_INT(EVENT_FRAME_ERR_PARAM, EventFrame_PackText(NULL, text_buf, sizeof(text_buf), &packed_len));

    // 2. Text NULL buffer
    TEST_ASSERT_EQUAL_INT(EVENT_FRAME_ERR_PARAM, EventFrame_PackText(&event, NULL, sizeof(text_buf), &packed_len));

    // 3. Text NULL packed_len
    TEST_ASSERT_EQUAL_INT(EVENT_FRAME_ERR_PARAM, EventFrame_PackText(&event, text_buf, sizeof(text_buf), NULL));

    // 4. Binary NULL event
    TEST_ASSERT_EQUAL_INT(EVENT_FRAME_ERR_PARAM, EventFrame_PackBinary(NULL, bin_buf, sizeof(bin_buf), &packed_len));

    // 5. Binary NULL buffer
    TEST_ASSERT_EQUAL_INT(EVENT_FRAME_ERR_PARAM, EventFrame_PackBinary(&event, NULL, sizeof(bin_buf), &packed_len));

    // 6. Binary NULL packed_len
    TEST_ASSERT_EQUAL_INT(EVENT_FRAME_ERR_PARAM, EventFrame_PackBinary(&event, bin_buf, sizeof(bin_buf), NULL));

    printf("--> Validación de parámetros nulos completada exitosamente.\n");
}

/* ============================================================================
 * TEST-MW-FRAME-02: Validación de buffers insuficientes (Too Small)
 * ============================================================================ */
TEST_CASE("TEST-MW-FRAME-02 EventFrame buffer too small validation", "[middleware][event_frame]") {
    printf("\n========================================\n");
    printf(" TEST-MW-FRAME-02 BUFFER INSUFICIENTE\n");
    printf("========================================\n");

    event_data_t event = {
        .event_type = EVENT_TYPE_PANIC_ALERT,
        .sequence_number = 123
    };
    strncpy(event.imei, TEST_IMEI, sizeof(event.imei));

    char small_text_buf[10];  // Insuficiente para "01,123456789012345,123\r\n"
    uint8_t small_bin_buf[15]; // Insuficiente para los 20 bytes requeridos
    uint16_t packed_len = 0;

    // Pack Text con buffer insuficiente
    TEST_ASSERT_EQUAL_INT_MESSAGE(EVENT_FRAME_ERR_BUFFER_TOO_SMALL,
        EventFrame_PackText(&event, small_text_buf, sizeof(small_text_buf), &packed_len),
        "EventFrame_PackText debió fallar por buffer pequeño");

    // Pack Binary con buffer insuficiente (< 20 bytes)
    TEST_ASSERT_EQUAL_INT_MESSAGE(EVENT_FRAME_ERR_BUFFER_TOO_SMALL,
        EventFrame_PackBinary(&event, small_bin_buf, sizeof(small_bin_buf), &packed_len),
        "EventFrame_PackBinary debió fallar por buffer pequeño");

    printf("--> Rechazo por buffers insuficientes verificado correctamente.\n");
}

/* ============================================================================
 * TEST-MW-FRAME-03: Serialización exitosa en texto (CSV)
 * ============================================================================ */
TEST_CASE("TEST-MW-FRAME-03 EventFrame_PackText successful CSV formatting", "[middleware][event_frame]") {
    printf("\n========================================\n");
    printf(" TEST-MW-FRAME-03 SERIALIZACION CSV EN TEXTO\n");
    printf("========================================\n");

    event_data_t event = {
        .event_type = EVENT_TYPE_PANIC_ALERT,
        .sequence_number = 1
    };
    strncpy(event.imei, TEST_IMEI, sizeof(event.imei));

    char buffer[64] = {0};
    uint16_t packed_len = 0;

    event_frame_err_t err = EventFrame_PackText(&event, buffer, sizeof(buffer), &packed_len);

    TEST_ASSERT_EQUAL_INT(EVENT_FRAME_OK, err);
    TEST_ASSERT_EQUAL_STRING("01,123456789012345,1\r\n", buffer);
    TEST_ASSERT_EQUAL_UINT16(strlen("01,123456789012345,1\r\n"), packed_len);

    printf("--> Trama CSV generada: %s", buffer);
    printf("--> Longitud reportada: %u bytes\n", packed_len);
    printf("--> Pruebas de empaquetado CSV exitosas.\n");
}

/* ============================================================================
 * TEST-MW-FRAME-04: Serialización exitosa en formato binario (20 bytes)
 * ============================================================================ */
TEST_CASE("TEST-MW-FRAME-04 EventFrame_PackBinary successful binary packing", "[middleware][event_frame]") {
    printf("\n========================================\n");
    printf(" TEST-MW-FRAME-04 SERIALIZACION BINARIA COMPACTA\n");
    printf("========================================\n");

    event_data_t event = {
        .event_type = EVENT_TYPE_PANIC_ALERT,
        .sequence_number = 0x0102 // 258 en decimal
    };
    strncpy(event.imei, TEST_IMEI, sizeof(event.imei));

    uint8_t buffer[32] = {0};
    uint16_t packed_len = 0;

    event_frame_err_t err = EventFrame_PackBinary(&event, buffer, sizeof(buffer), &packed_len);

    TEST_ASSERT_EQUAL_INT(EVENT_FRAME_OK, err);
    TEST_ASSERT_EQUAL_UINT16(20, packed_len);

    // 1. Validar Header Magic (0xAA55)
    TEST_ASSERT_EQUAL_HEX8(0xAA, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x55, buffer[1]);

    // 2. Validar Tipo de Evento (0x01)
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer[2]);

    // 3. Validar IMEI ASCII (15 bytes)
    TEST_ASSERT_EQUAL_MEMORY(TEST_IMEI, &buffer[3], 15);

    // 4. Validar Secuencia Big Endian (0x01, 0x02)
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer[18]);
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer[19]);

    printf("--> Header Magic: 0x%02X%02X\n", buffer[0], buffer[1]);
    printf("--> Tipo de Evento: 0x%02X\n", buffer[2]);
    printf("--> Secuencia Big Endian: 0x%02X%02X\n", buffer[18], buffer[19]);
    printf("--> Empaquetado binario de 20 bytes verificado correctamente.\n");
}