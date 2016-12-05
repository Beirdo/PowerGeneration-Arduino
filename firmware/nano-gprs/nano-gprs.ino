#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "rflink.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"

// in ms
#define LOOP_CADENCE 1000

#define RF_CS_PIN 10
#define RF_CE_PIN 9
#define RF_IRQ_PIN 2

#define DTR_PIN 3

static const uint8_t EEMEM rf_link_id = 0;

RFLink *rflink = NULL;
SleepTimer sleepTimer(LOOP_CADENCE);

void CborMessageBuild(void);

void CborMessageBuild(void)
{
    CborMessageInitialize();
    CborMessageAddMap(3);
    CborMapAddInteger(CBOR_KEY_SOURCE, CBOR_SOURCE_NANO_GPRS);
    CborMapAddInteger(CBOR_KEY_CORE_TEMPERATURE, core_temperature);
    CborMapAddInteger(CBOR_KEY_GPRS_RSSI, gprs_rssi);
}


void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;
    
    Serial.begin(115200);

    cli.initialize();

    uint8_t rf_id = EEPROM.read(rf_link_id);

    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id);
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    noInterrupts();
    sleepTimer.enable();

    core_temperature = readAvrTemperature();

    CborMessageBuild();
    CborMessageBuffer(&buffer, &len);
    if (buffer && len) {
        rflink->send(buffer, len);
    }

    cli.handleInput();

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
