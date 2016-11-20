#ifndef TEMPERATURES_H__
#define TEMPERATURES_H__

#include <inttypes.h>

void TemperaturesInitialize(void);
void TemperaturesPoll(uint16_t *values, uint8_t count);

#endif
