#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <Adafruit_FRAM_SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "rflink.h"
#include "temperatures.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"
#include "lcdscreen.h"
#include "eeprom.h"

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

#define FRAM_CS_PIN 8

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

uint16_t temperatures[8];

static const eeprom_t EEMEM eeprom_contents = { 0 };
uint8_t rf_id;

uint32_t battery_voltage;

RFLink *rflink = NULL;
SleepTimer sleepTimer(LOOP_CADENCE);

Adafruit_FRAM_SPI fram(FRAM_CS_PIN);
Adafruit_SSD1306 oled;
LCDDeck lcdDeck(&oled, true);

void CborMessageBuild(void);

void CborMessageBuild(void)
{
    CborMessageInitialize();
    CborMessageAddMap(3);
    CborMapAddInteger(CBOR_KEY_SOURCE, CBOR_SOURCE_NANO_TEMPERATURE);
    CborMapAddArray(CBOR_KEY_TEMPERATURE_ARRAY, temperatures, 8);
    CborMapAddInteger(CBOR_KEY_CORE_TEMPERATURE, core_temperature);
}


class InitializeLogoCLICommand : public CLICommand
{
    public:
        InitializeLogoCLICommand(void) : CLICommand("initlogo", 0) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                Serial.println("Writing logo to FRAM");
                oled.initializeLogo();
                Serial.println("Done");
                return 1;
            };
};



class GetRFIDCLICommand : public CLICommand
{
    public:
        GetRFIDCLICommand(void) : CLICommand("get_rf_link", 0) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t rf_id = EEPROM.read(EEPROM_OFFSET(rf_link_id));
                Serial.print("Current RF ID = ");
                Serial.println(rf_id, HEX);
                return 1;
            };
};

class SetRFIDCLICommand : public CLICommand
{
    public:
        SetRFIDCLICommand(void) : CLICommand("set_rf_link", 1) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t rf_id = (uint8_t)(strtoul(args[0], 0, 16) & 0xFF);
                EEPROM.update(EEPROM_OFFSET(rf_link_id), rf_id);
                Serial.print("New RF ID = ");
                Serial.println(rf_id, HEX);
                return 1;
            };
};


void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;
    
    Serial.begin(115200);

    cli.registerCommand(new GetRFIDCLICommand());
    cli.registerCommand(new SetRFIDCLICommand());
    cli.registerCommand(new InitializeLogoCLICommand());
    cli.initialize();

    rf_id = EEPROM.read(EEPROM_OFFSET(rf_link_id));

    bool framInit = fram.begin();
    if (!framInit) {
        Serial.println("Can't find attached FRAM");
    }
    
    oled.begin(SSD1306_SWITCHCAPVCC);
    if (framInit) {
        oled.attachRAM(&fram, 0x0000, 0x04000);
    }
    oled.display();

    lcdDeck.addFrame(new LCDScreen("Core Temp",
                     (void *)&core_temperature, formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Battery",
                     (void *)&battery_voltage, formatAutoScale, "V"));

    lcdDeck.addFrame(new LCDScreen("Temp 1", (void *)&temperatures[0],
                     formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Temp 2", (void *)&temperatures[1],
                     formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Temp 3", (void *)&temperatures[2],
                     formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Temp 4", (void *)&temperatures[3],
                     formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Temp 5", (void *)&temperatures[4],
                     formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Temp 6", (void *)&temperatures[5],
                     formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Temp 7", (void *)&temperatures[6],
                     formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Temp 8", (void *)&temperatures[7],
                     formatTemperature, "C"));

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
