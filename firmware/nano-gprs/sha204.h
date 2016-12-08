#ifndef SHA204_H__
#define SHA204_H__

void Sha204Initialize(void);
byte Sha204Wakeup(void);
byte Sha204GetSerialNumber(void);
byte Sha204MacChallenge(void);

#endif

// vim:ts=4:sw=4:ai:et:si:sts=4
