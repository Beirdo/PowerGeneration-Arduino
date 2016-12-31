#ifndef EEPROM_COMMON_H__
#define EEPROM_COMMON_H__

#ifdef __arm__
#define EEMEM
#else
#include <EEPROM.h>
#include <avr/eeprom.h>
#endif
#include <inttypes.h>

struct _eeprom_t;
typedef struct _eeprom_t __attribute__((packed, aligned(1))) eeprom_t;

#define EEPROM_OFFSET(x)  (OFFSETOF(eeprom_t, x))
#define OFFSETOF(s, x)    ((size_t)(&((s *)NULL)->x))

#ifdef __arm__
class SimulatedEEPROM {
    public:
        SimulatedEEPROM(const eeprom_t *eeprom, uint16_t len)
        {
            m_eeprom = new uint8_t[len];
            m_len = len;
            memcpy(m_eeprom, eeprom, len);
        }
        uint8_t read(uint16_t offset)
        { 
            return m_eeprom[offset]; 
        }
        void update(uint16_t offset, uint8_t value)
        {
            m_eeprom[offset] = value;
        }
    protected:
        uint8_t *m_eeprom;
        uint16_t m_len;
};

extern SimulatedEEPROM EEPROM;
#endif

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
