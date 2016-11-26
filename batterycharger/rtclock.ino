#include <Wire.h>
#include <PCF2129.h>

#include "rtclock.h"

PCF2129 rtc(PCF2129_SLAVE_ADDRESS, 8);
DateTime now;

void RTClockInitialize(void)
{
  if (!rtc.searchDevice()) {
    Serial.println("RTC device not found!");
    while(1);
  }
  rtc.configure();
}

void RTClockPoll(void)
{
  if (rtc.getPoll()) {
    now = rtc.now();
  }
}

uint16_t parseNumber(uint8_t **string, uint8_t terminator);

uint16_t parseNumber(uint8_t **string, uint8_t terminator)
{
  uint8_t *ch;
  uint8_t digit;
  uint16_t value = 0;
  
  for (ch = *string; *ch && *ch != terminator; ch++) {
    digit = *ch;
    if (digit >= '0' && digit <= '9') {
      digit -= '0';
      value *= 10;
      value += digit;
    }
  }
  if (*ch) {
    // skip the terminator
    ch++;
  }
  *string = ch;
  return value;
}

void RTClockSet(uint8_t *datestring)
{
  // Format: HH:MM:SS YYYY/mm/dd
  uint8_t *ch;
  uint8_t hours, minutes, seconds, months, days;
  uint16_t years;

  ch = datestring;
  hours = parseNumber(&ch, ':');
  minutes = parseNumber(&ch, ':');
  seconds = parseNumber(&ch, ' ');
  years = parseNumber(&ch, '/');
  months = parseNumber(&ch, '/');
  days = parseNumber(&ch, ' ');

  rtc.setDate(years, months, days, hours, minutes, seconds);
}

DateTime RTClockGetTime(void)
{
  return now;
}

