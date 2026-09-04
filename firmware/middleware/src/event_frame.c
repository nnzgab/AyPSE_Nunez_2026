#include "event_frame.h"
#include "cellular.h"
#include <stdio.h>
#include <string.h>

#define MAX_TCP_RETRIES     10
//#define SERVER_HOST         "192.168.1.100"
//#define SERVER_PORT         5000
#define SERVER_HOST         "nzmgf-190-183-23-94.run.pinggy-free.link"
#define SERVER_PORT         40309
//#define BACKUP_PHONE        "+5493794000000" /* Número de destino por SMS */

static uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return ~crc;
}

bool EventFrameInit(void) {
    return true;
}

bool EventFrameBuild(uint8_t status, const char *token, uint8_t *out_buffer, size_t *out_len) {
    if (!out_buffer || !out_len || *out_len == 0) {
        return false;
    }

    char imei[32];
    char timestamp[32];
    char raw_data[128];

    /* Consultar directamente a la capa de telefonía celular */
    if (!CellularGetImei(imei, sizeof(imei))) {
        snprintf(imei, sizeof(imei), "UNKNOWN_IMEI");
    }

    if (!CellularGetNtpTime(timestamp, sizeof(timestamp))) {
        snprintf(timestamp, sizeof(timestamp), "2026-06-18 00:00:00");
    }

    int raw_len = snprintf(raw_data, sizeof(raw_data), "%s|%s|%s|%u", token, imei, timestamp, status);
    if (raw_len < 0 || (size_t)raw_len >= sizeof(raw_data)) {
        return false;
    }

    uint32_t crc = calculate_crc32((const uint8_t *)raw_data, (size_t)raw_len);

    int total_len = snprintf((char *)out_buffer, *out_len, "CRC:%08X|%s\r\n", (unsigned int)crc, raw_data);
    if (total_len < 0 || (size_t)total_len >= *out_len) {
        return false;
    }

    *out_len = (size_t)total_len;
    return true;
}

bool EventFrameDispatch(const uint8_t *buffer, size_t length) {
    if (!buffer || length == 0) {
        return false;
    }

    uint8_t retries = 0;
    bool tcp_success = false;

    while (retries < MAX_TCP_RETRIES && !tcp_success) {
        //if (CellularTcpConnect(SERVER_HOST, SERVER_PORT)) {
        if (CellularOpenSocket(0, "TCP", SERVER_HOST, SERVER_PORT)) {
            //if (CellularTcpSend(buffer, length)) {
            if (CellularSendTcp(0, (const char *)buffer, length)) {
                tcp_success = true;
            }
            //CellularTcpDisconnect();
            CellularCloseSocket(0);
        }

        if (!tcp_success) {
            retries++;
        }
    }

    if (tcp_success) {
        return true;
    }

    /* Respaldo por SMS si falla el canal TCP */
    char sms_message[160];
    snprintf(sms_message, sizeof(sms_message), "ALERTA CRITICA! Falla de red TCP.");
    return CellularSmsSend(BACKUP_PHONE, sms_message);
}