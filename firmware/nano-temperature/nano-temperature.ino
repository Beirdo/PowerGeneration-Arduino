#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "rflink.h"
#include "temperatures.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"

// in ms
#define LOOP_CADENCE 1000
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

uint16_t lcdTicks;
int8_t lcdIndex;

#define RF_CS_PIN 10
#define RF_CE_PIN 9
#define RF_IRQ_PIN 2

#define VBATT_ADC_PIN 7

#define OLED_RESET -1

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

uint16_t temperatures[8];

static const uint8_t EEMEM rf_link_id = 0;
uint8_t rf_id;

uint32_t battery_voltage;

RFLink *rflink = NULL;
SleepTimer sleepTimer(LOOP_CADENCE);

Adafruit_SSD1306 LCD(OLED_RESET);
LCDDeck lcdDeck(&LCD);

void CborMessageBuild(void);

void CborMessageBuild(void)
{
    CborMessageInitialize();
    CborMessageAddMap(3);
    CborMapAddInteger(CBOR_KEY_SOURCE, CBOR_SOURCE_NANO_TEMPERATURE);
    CborMapAddArray(CBOR_KEY_TEMPERATURE_ARRAY, temperatures, 8);
    CborMapAddInteger(CBOR_KEY_CORE_TEMPERATURE, core_temperature);
}

void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;
    
    Serial.begin(115200);

    cli.initialize();

    rf_id = EEPROM.read(rf_link_id);

    LCD.begin(SSD1306_SWITCHCAPVCC, 0x3D);
    LCD.display();

    lcdDeck.addFrame(new LCDScreen("Core Temp",
                     (void *)&core_temperature, formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Battery",
                     (void *)&battery_voltage, formatAutoScale, "V"));

    lcdDeck.addFrame(new LCDScreen("Temp 1", (void *)&temperatures[0],
                     formatTemperature, "C");
    lcdDeck.addFrame(new LCDScreen("Temp 2", (void *)&temperatures[1],
                     formatTemperature, "C");
    lcdDeck.addFrame(new LCDScreen("Temp 3", (void *)&temperatures[2],
                     formatTemperature, "C");
    lcdDeck.addFrame(new LCDScreen("Temp 4", (void *)&temperatures[3],
                     formatTemperature, "C");
    lcdDeck.addFrame(new LCDScreen("Temp 5", (void *)&temperatures[4],
                     formatTemperature, "C");
    lcdDeck.addFrame(new LCDScreen("Temp 6", (void *)&temperatures[5],
                     formatTemperature, "C");
    lcdDeck.addFrame(new LCDScreen("Temp 7", (void *)&temperatures[6],
                     formatTemperature, "C");
    lcdDeck.addFrame(new LCDScreen("Temp 8", (void *)&temperatures[7],
                     formatTemperature, "C");

    lcdTicks = 0;

    TemperaturesInitialize();
    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id);
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    noInterrupts();
    sleepTimer.enable();

    lcdTicks++;
    if (lcdTicks >= SWAP_TIME) {
        lcdTicks -= SWAP_TIME;

        core_temperature = readAvrTemperature();

        analogReference(DEFAULT);
        battery_voltage = map(analogRead(VBATT_ADC_PIN), 0, 1023, 0, 5000);

        TemperaturesPoll(temperatures, 8);

        lcdIndex = lcdDeck.nextIndex();
        lcdDeck.formatFrame(lcdIndex);
        lcdDeck.displayFrame();

        CborMessageBuild();
        CborMessageBuffer(&buffer, &len);
        if (buffer && len) {
            rflink->send(buffer, len);
        }
    }

    cli.handleInput();

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
