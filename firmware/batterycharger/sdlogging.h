#ifndef SDLOGGING_H__
#define SDLOGGING_H__

void SDCardInitialize(uint8_t cs);
void SDCardWrite(uint8_t *buffer, uint8_t len);

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
