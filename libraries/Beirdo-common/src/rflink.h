#ifndef RFLINK_H__
#define RFLINK_H__

#include <RF24.h>
#include <inttypes.h>

class RFLink {
    public:
        RFLink(uint8_t ce, uint8_t cs, uint8_t irq, uint8_t nodeNum);
        void send(const void *buf, uint8_t len);
        uint8_t receive(void *buf, uint8_t maxlen, uint8_t *pipeNum);
    protected:
        uint8_t m_nodeNum;
        uint8_t m_irq;
        RF24 m_rf;
        uint8_t m_valid;
};

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
