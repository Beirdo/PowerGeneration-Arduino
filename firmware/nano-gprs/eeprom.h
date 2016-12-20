#ifndef EEPROM_H__
#define EEPROM_H__

#include <EEPROM.h>
#include <avr/eeprom.h>
#include <inttypes.h>
#include <eeprom_common.h>

typedef uint8_t apn_t[MAX_APN_LEN];

struct _eeprom_t {
    uint8_t rf_link_id;
    apn_t gprs_apn;
};

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
