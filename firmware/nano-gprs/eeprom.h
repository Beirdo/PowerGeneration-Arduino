#ifndef EEPROM_H__
#define EEPROM_H__

#include <EEPROM.h>
#include <avr/eeprom.h>
#include <inttypes.h>

typedef uint8_t apn_t[MAX_APN_LEN];

typedef struct {
    uint8_t rf_link_id;
    apn_t gprs_apn;
} __attribute__((packed, aligned(1))) eeprom_t;

#define EEPROM_OFFSET(x)  (OFFSETOF(eeprom_t, (x)))
#define OFFSET(s, x)      ((uint16_t)(&((s *)NULL)->(x)))

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
