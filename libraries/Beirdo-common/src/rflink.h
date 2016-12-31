#ifndef RFLINK_H__
#define RFLINK_H__

#include <RF24.h>
#include <inttypes.h>
#include "eeprom_common.h"

class RFLink {
    public:
        RFLink(uint8_t ce, uint8_t cs, uint8_t irq, uint8_t nextHop,
               uint8_t upstreamBase);
        void send(const void *buf, uint8_t len);
        uint8_t receive(void *buf, uint8_t maxlen, uint8_t *pipeNum);
    protected:
        RF24 m_rf;
        uint8_t m_nextHop;
        uint8_t m_upstreamBase;
        uint8_t m_irq;
        bool m_listening;
        bool m_valid;
};

extern const uint8_t EEMEM rf_link_id;
extern const uint8_t EEMEM rf_link_upstream;

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
