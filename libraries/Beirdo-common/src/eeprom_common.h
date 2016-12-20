#ifndef EEPROM_COMMON_H__
#define EEPROM_COMMON_H__

#include <EEPROM.h>
#include <avr/eeprom.h>
#include <inttypes.h>

struct _eeprom_t;
typedef struct _eeprom_t __attribute__((packed, aligned(1))) eeprom_t;

#define EEPROM_OFFSET(x)  (OFFSETOF(eeprom_t, x))
#define OFFSETOF(s, x)    ((uint16_t)(&((s *)NULL)->x))

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
