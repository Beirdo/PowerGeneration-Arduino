#ifndef RTCLOCK_H__
#define RTCLOCK_H__

#include <PCF2129.h>

void RTClockInitialize(void);
void RTClockPoll(void);
void RTClockSet(uint8_t *datestring);
DateTime RTClockGetTime(void);

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
