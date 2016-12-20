#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9340.h>

#include "rflink.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"
#include "lcdscreen.h"
#include "sha204.h"
#include "gprs.h"

// in ms
#define LOOP_CADENCE 1000
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

uint16_t lcdTicks;
int8_t lcdIndex;

#define RF_CS_PIN 10
#define RF_CE_PIN 9
#define RF_IRQ_PIN 2

#define GPRS_RST_PIN 4
#define GPRS_EN_PIN 5
#define GPRS_DTR_PIN 3

#define LCD_RST_PIN 7
#define LCD_CS_PIN 8
#define LCD_DC_PIN 16
#define LCD_PWM_PIN 6

#define SD_CS_PIN 17

#define VBATT_ADC_PIN 7
#define LIGHT_ADC_PIN 6

typedef uint8_t apn_t[MAX_APN_LEN];
static const uint8_t EEMEM rf_link_id = 0;
static const apn_t EEMEM ee_gprs_apn;

uint32_t battery_voltage;

RFLink *rflink = NULL;
SleepTimer sleepTimer(LOOP_CADENCE);

apn_t gprs_apn;
GPRS gprs(GPRS_RST_PIN, GPRS_EN_PIN, GPRS_DTR_PIN);

Adafruit_ILI9340 LCD(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);
LCDDeck lcdDeck(&LCD, false);

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

    analogReference(DEFAULT);

    LCD.begin();
    LCD.setRotation(1);     // use in landscape mode

    lcdDeck.addFrame(new LCDScreen("Core Temp",
                     (void *)&core_temperature, formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Battery",
                     (void *)&battery_voltage, formatAutoScale, "V"));

    lcdTicks = 0;

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
        lcdTicks++;
        if (lcdTicks >= SWAP_TIME) {
            lcdTicks -= SWAP_TIME;

            core_temperature = readAvrTemperature();
            
            analogReference(DEFAULT);
            battery_voltage = map(analogRead(VBATT_ADC_PIN), 0, 1023, 0, 5000);
            int16_t ledValue = map(analogRead(LIGHT_ADC_PIN), 204, 819, 0, 255);
            analogWrite(LED_PWM_PIN, (uint8_t)constrain(ledValue, 0, 255));

            lcdIndex = lcdDeck.nextIndex();
            lcdDeck.formatFrame(lcdIndex);
            lcdDeck.displayFrame();

            CborMessageBuild();
            CborMessageBuffer(&buffer, &len);
            if (buffer && len) {
                gprs.sendCborPacket(CBOR_SOURCE_NANO_GPRS, buffer, len);
            }
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
