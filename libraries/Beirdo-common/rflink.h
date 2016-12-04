#ifndef RFLINK_H__
#define RFLINK_H__

void RFLinkInitialize(uint8_t irq, uint8_t nodeNum);
void RFLinkSend(const void *buf, uint8_t len);
uint8_t RFLinkReceive(void *buf, uint8_t maxlen);

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
