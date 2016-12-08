#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "rflink.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"
#include "sha204.h"
#include "gprs.h"

// in ms
#define LOOP_CADENCE 1000

#define RF_CS_PIN 10
#define RF_CE_PIN 9
#define RF_IRQ_PIN 2

#define GPRS_RST_PIN 4
#define GPRS_EN_PIN 5
#define GPRS_DTR_PIN 3

typedef uint8_t apn_t[MAX_APN_LEN];
static const uint8_t EEMEM rf_link_id = 0;
static const apn_t EEMEM ee_gprs_apn;

RFLink *rflink = NULL;
SleepTimer sleepTimer(LOOP_CADENCE);

apn_t gprs_apn;
GPRS gprs(GPRS_RST_PIN, GPRS_EN_PIN, GPRS_DTR_PIN);

#define RF_RX_BUFFER_SIZE 64
uint8_t rf_rx_buffer[RF_RX_BUFFER_SIZE];

void CborMessageBuild(void);

void CborMessageBuild(void)
{
    CborMessageInitialize();
    CborMessageAddMap(3);
    CborMapAddInteger(CBOR_KEY_SOURCE, CBOR_SOURCE_NANO_GPRS);
    CborMapAddInteger(CBOR_KEY_CORE_TEMPERATURE, core_temperature);
    CborMapAddInteger(CBOR_KEY_GPRS_RSSI, gprs.getRssi());
}

// TODO:  CLI commands for:
//            get/set APN
//            get/set URL (from SHA204)
//            get/set AWS Creds (from SHA204)

void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;

    if (gprs.isDisabled()) {
        Serial.begin(115200);
        cli.initialize();
    }

    uint8_t rf_id = EEPROM.read(rf_link_id);
    EEPROM.get(ee_gprs_apn, gprs_apn);
    gprs.setApn(gprs_apn);

    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id);

    Sha204Initialize();
}

void loop() 
{
    uint8_t *buffer = NULL;
    uint8_t len = 0;
    static bool gprsDisabled;

    noInterrupts();
    sleepTimer.enable();

    core_temperature = readAvrTemperature();

    bool newDisabled = gprs.isDisabled();
    if (newDisabled != gprsDisabled) {
        gprsDisabled = newDisabled;

        if (gprsDisabled) {
            Serial.begin(115200);
            cli.initialize();
        }
    }

    if (gprsDisabled) {
        cli.handleInput();
    } else {
        CborMessageBuild();
        CborMessageBuffer(&buffer, &len);
        if (buffer && len) {
            gprs.sendCborPacket(CBOR_SOURCE_NANO_GPRS, buffer, len);
        }

        uint8_t source;
        len = rflink.receive(rf_rx_buffer, RF_RX_BUFFER_SIZE, &source);
        if (len) {
            gprs.sendCborPacket(source, rf_rx_buffer, len);
        }
    }

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
