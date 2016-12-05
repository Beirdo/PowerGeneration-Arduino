#ifndef RTCLOCK_H__
#define RTCLOCK_H__

#include <PCF2129.h>

void RTClockInitialize(void);
void RTClockPoll(void);
void RTClockSet(uint8_t *datestring);
DateTime RTClockGetTime(void);

#endif
