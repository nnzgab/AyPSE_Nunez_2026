#include "event_frame.h"
#include "cellular_modem.h"
#include <stdio.h>
#include <string.h>

#define MAX_TCP_RETRIES     10
//#define SERVER_HOST         "192.168.1.100"
//#define SERVER_PORT         5000
//bftnb-190-183-23-94.run.pinggy-free.link:44331 
#define SERVER_HOST         "bftnb-190-183-23-94.run.pinggy-free.link"
#define SERVER_PORT         44331
//#define BACKUP_PHONE        "+5493794000000" /* Número de destino por SMS */


static const char *EventTypeToString(event_type_t type)
{
    switch (type)
    {
        case EVENT_TYPE_PANIC:
            return "PANIC";
        default:
            return "UNKNOWN";
    }
}

int EventFrameBuild(const event_frame_t *frame, char *buffer_out, size_t max_len)
{
    if (frame == NULL || buffer_out == NULL || max_len == 0)
    {
        return -1;
    }

    int written = snprintf(buffer_out, max_len, "%s,%s",
                            EventTypeToString(frame->type), frame->imei);

    if (written < 0 || (size_t)written >= max_len)
    {
        return -1; /* no entró en el buffer */
    }

    return written;
}