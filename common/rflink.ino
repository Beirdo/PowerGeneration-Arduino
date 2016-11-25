#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

#include <SPI.h>

#include "rflink.h"

RF24 radio(RF_CE_PIN, RF_CS_PIN);
uint8_t rf_valid = 0;

const uint64_t rf_pipe_base = 0xC0FFEE0000LL;

void RFLinkInitialize(uint8_t irq, uint8_t nodeNum)
{
  // ignore irq for now
  
  radio.begin();
  radio.enableDynamicPayloads();

  if (nodeNum == 0xFE) {
    // Master
    for (uint8_t i = 0; i < 6; i++) {
      radio.openReadingPipe(i + 1, rf_pipe_base + i);
    }
    radio.startListening();
  } else if (nodeNum >= 6) {
    return;
  } else {
    radio.openWritingPipe(rf_pipe_base + nodeNum + 1);
  }

  rf_valid = 1;
}

void RFLinkSend(const void *buf, uint8_t len)
{
  if (!rf_valid) {
    return;
  }
  radio.write(buf, len);
}

uint8_t RFLinkReceive(void *buf, uint8_t maxlen)
{
  uint8_t pipeNum;
  uint8_t pktlen;

  if (!rf_valid || !radio.available(&pipeNum)) {
    return 0;
  }

  pktlen = radio.getDynamicPayloadSize();
  if (pktlen > maxlen) {
    pktlen = maxlen;
  }
  radio.read(buf, pktlen);
  maxlen = pktlen;
  return pktlen;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
