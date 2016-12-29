#include <Arduino.h>
#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

#include <SPI.h>

#include "rflink.h"

const uint32_t rf_pipe_base = 0xC0FFEE00L;

RFLink::RFLink(uint8_t ce, uint8_t cs, uint8_t irq, uint8_t nextHop,
               uint8_t upstreamBase) : 
    m_rf(RF24(ce, cs))
{
    m_nextHop = nextHop;
    if (upstreamBase == 0xFF) {
        m_upstreamBase = upstreamBase;
    } else {
        m_upstreamBase = upstreamBase & 0xF8;
    }
    m_irq = irq;
    m_listening = false;
    m_valid = false;

    // ignore irq for now
    
    m_rf.begin();
    m_rf.setAddressWidth(4);
    m_rf.enableDynamicPayloads();
    m_rf.setAutoAck(true);

    // We read from the upstream, and relay to nextHop
    if (m_upstreamBase != 0xFF) {
        for (uint8_t i = 0; i < 6; i++) {
            m_rf.openReadingPipe(i, rf_pipe_base + m_upstreamBase + i);
        }
        m_rf.startListening();
        m_listening = true;
    }

    m_valid = 1;
}

void RFLink::send(const void *buf, uint8_t len)
{
    if (!m_valid || m_nextHop == 0xFF || m_nextHop == 0xFE) {
        return;
    }

    if (m_listening) {
        m_rf.stopListening();
        m_listening = false;
    }

    // Reopen pipe 0 to send to the next hop
    m_rf.openWritingPipe(rf_pipe_base + m_nextHop);
    m_rf.write(buf, len);

    // Reopen pipe 0 to listen to upstream
    if (m_upstreamBase != 0xFF) {
        m_rf.openReadingPipe(0, rf_pipe_base + m_upstreamBase);
        m_rf.startListening();
        m_listening = true;
    }
}

uint8_t RFLink::receive(void *buf, uint8_t maxlen, uint8_t *pipeNum)
{
    uint8_t pktlen = 0;

    if (m_valid && m_rf.available(pipeNum)) {
        pktlen = m_rf.getDynamicPayloadSize();
        if (pktlen != 0) {
            pktlen = min(pktlen, maxlen);
            m_rf.read(buf, pktlen);

            // Send the message on to the next hop
            if (m_nextHop != 0xFF && m_nextHop != 0xFE) {
                send(buf, pktlen);
                pktlen = 0;
            }
        }
    }
    return pktlen;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
