#include <EEPROM.h>
#include <avr/eeprom.h>
#include <LowPower.h>
#include <Adafruit_FRAM_SPI.h>
#include <Adafruit_GFX.h>
#include <SSD1306.h>

#include "rflink.h"
#include "temperatures.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"
#include "lcdscreen.h"
#include "eeprom.h"
#include "utils.h"

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
#error("Height incorrect, please fix SSD1306.h!");
#endif

SerialCLI cli(Serial);

ADCRead adcread;
int16_t core_temperature;
uint16_t temperatures[8];

static const eeprom_t EEMEM eeprom_contents = { 0xFF, 0xFF };
uint8_t rf_id;
uint8_t rf_upstream;

uint32_t battery_voltage;

RFLink *rflink = NULL;

Adafruit_FRAM_SPI fram(FRAM_CS_PIN);
SSD1306 oled;
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
                cli.serial()->println("Writing logo to FRAM");
                oled.initializeLogo();
                cli.serial()->println("Done");
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
                cli.serial()->print("Current RF ID = ");
                cli.serial()->println(rf_id, HEX);
                return 1;
            };
};

class SetRFIDCLICommand : public CLICommand
{
    public:
        SetRFIDCLICommand(void) : CLICommand("set_rf_link", 1) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t rf_id = atou8(args[0]);
                EEPROM.update(EEPROM_OFFSET(rf_link_id), rf_id);
                cli.serial()->print("New RF ID = ");
                cli.serial()->println(rf_id, HEX);
                return 1;
            };
};

class GetRFUpstreamCLICommand : public CLICommand
{
    public:
        GetRFUpstreamCLICommand(void) : CLICommand("get_rf_upstream", 0) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t rf_up = EEPROM.read(EEPROM_OFFSET(rf_link_upstream));
                cli.serial()->print("Current RF Upstream = ");
                cli.serial()->println(rf_up, HEX);
                return 1;
            };
};

class SetRFUpstreamCLICommand : public CLICommand
{
    public:
        SetRFUpstreamCLICommand(void) : CLICommand("set_rf_upstream", 1) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t rf_up = atou8(args[0]);
                EEPROM.update(EEPROM_OFFSET(rf_link_upstream), rf_up);
                cli.serial()->print("New RF Upstream = ");
                cli.serial()->println(rf_up, HEX);
                return 1;
            };
};


void setup() 
{
    cli.registerCommand(new GetRFIDCLICommand());
    cli.registerCommand(new SetRFIDCLICommand());
    cli.registerCommand(new GetRFUpstreamCLICommand());
    cli.registerCommand(new SetRFUpstreamCLICommand());
    cli.registerCommand(new InitializeLogoCLICommand());
    cli.initialize();

    rf_id = EEPROM.read(EEPROM_OFFSET(rf_link_id));
    rf_upstream = EEPROM.read(EEPROM_OFFSET(rf_link_upstream));

    bool framInit = fram.begin();
    if (!framInit) {
        cli.serial()->println("Can't find attached FRAM");
    }
    
    oled.begin(SSD1306_SWITCHCAPVCC);
    if (framInit) {
        oled.attachRAM(&fram, 0x0000, 0x0400);
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
    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id, rf_upstream);
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    lcdTicks++;
    if (lcdTicks >= SWAP_COUNT) {
        lcdTicks -= SWAP_COUNT;

        core_temperature = adcread.readCoreTemperature();
        battery_voltage = adcread.mapPin(VBATT_ADC_PIN, 0, 5000);

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

    LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                  SPI_OFF, USART0_ON, TWI_OFF);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
