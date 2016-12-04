#include <Arduino.h>
#include "bufprint.h"

#define digit(x)  ((char)(((x) + 0x30) & 0xFF))
#define split_value(value, part, scale)  \
  do { \
    part = value % scale; \
    value = value / scale; \
  } while(0)

void printTemperature(int value, char *buf, char maxlen)
{
  char index = maxlen - 1;

  // Temperatures don't need autoscaling
  char part;
  char sign = (value < 0 ? '-' : ' ');
  value = (value < 0 ? -value : value);

  split_value(value, part, 10);
  buf[index--] = digit(part);
  buf[index--] = '.';

  for (char i = 0; index >= 0 && (value != 0 || i == 0); i++) {
    split_value(value, part, 10);
    buf[index--] = digit(part);
  }

  buf[index] = sign;
}

void printValue(long value, char maxunits, char *buf, char maxlen)
{
  char index = maxlen - 1;

  // Power, voltage, current do need autoscaling
  if (value == 0) {
    buf[index--] = digit(0);
    return;
  }

  long tempdigit;
  char digitcount = 0;

  for (tempdigit = value; tempdigit; tempdigit /= 10) {
    digitcount++;
  }

  char units = maxunits - (digitcount / 3);;

  switch (units) {
    case 1:
      buf[index--] = 'm';
      break;
    case 2:
      buf[index--] = 'u';
      break;
    case 0:
      break;
    default:
      buf[index--] = digit(0);
      return;
  }

  char digits = digitcount % 3;
  digits = (digits ? digits : 3);

  char decimals = (index - 1 <= digits ? (index - digits) : 0);
  digits += decimals;

  long scale;
  for (scale = 1, digitcount = 0; digitcount < digits; scale *= 10) {
    digitcount++;
  }

  value /= scale;
  char newdigit;
  for (; digits > 0; digits--) {
    split_value(value, newdigit, 10);
    buf[index--] = digit(newdigit);
    if (decimals) {
      decimals--;
      if (!decimals) {
        buf[index--] = '.';
      }
    }
  }
}

// vim:ts=4:sw=4:ai:et:si:sts=4
