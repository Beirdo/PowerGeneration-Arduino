#include <Arduino.h>
#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

#include <SPI.h>

#include "rflink.h"

const uint64_t rf_pipe_base = 0xC0FFEE0000LL;

RFLink::RFLink(uint8_t ce, uint8_t cs, uint8_t irq, uint8_t nodeNum) : 
    m_rf(RF24(ce, cs))
{
    m_nodeNum = nodeNum;
    m_irq = irq;
    m_valid = 0;

    // ignore irq for now
    
    m_rf.begin();
    m_rf.enableDynamicPayloads();

    if (m_nodeNum == 0xFE) {
        // Master
        for (uint8_t i = 0; i < 6; i++) {
            m_rf.openReadingPipe(i + 1, rf_pipe_base + i);
        }
        m_rf.startListening();
    } else if (m_nodeNum >= 6) {
        return;
    } else {
        m_rf.openWritingPipe(rf_pipe_base + m_nodeNum + 1);
    }

    m_valid = 1;
}

void RFLink::send(const void *buf, uint8_t len)
{
    if (!m_valid) {
        return;
    }
    m_rf.write(buf, len);
}

uint8_t RFLink::receive(void *buf, uint8_t maxlen, uint8_t *pipeNum)
{
    uint8_t pktlen;

    if (!m_valid || !m_rf.available(pipeNum)) {
        return 0;
    }

    pktlen = m_rf.getDynamicPayloadSize();
    if (pktlen > maxlen) {
        pktlen = maxlen;
    }
    m_rf.read(buf, pktlen);
    maxlen = pktlen;
    return pktlen;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
