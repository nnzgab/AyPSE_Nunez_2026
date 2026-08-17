#ifndef QUECTEL_BSP_H
#define QUECTEL_BSP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

uint8_t Quectel_Init(void);
uint8_t Quectel_PowerOn(void);
uint8_t Quectel_PowerOff(void);
int Quectel_SendRaw(const char *buf, size_t len);
int Quectel_ReadRaw(char *buf, size_t maxlen, uint32_t timeout_ms);

#endif /* QUECTEL_BSP_H */
