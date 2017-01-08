#include <LowPower.h>
#include <Adafruit_FRAM_SPI.h>
#include <Adafruit_GFX.h>
#include <SSD1306.h>

#include "rflink.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"
#include "lcdscreen.h"
#include "sha204.h"
#include "gprs.h"
#include "eeprom.h"
#include "utils.h"
#include "sdlogging.h"

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
#define GPRS_DTR_PIN 3

#define SD_CS_PIN 7
#define SD_CD_PIN 8

#define FRAM_CS_PIN 6

#define VBATT_ADC_PIN 0

static const eeprom_t EEMEM eeprom_contents = { 0xFF, 0xFF };
#ifdef __arm__
SimulatedEEPROM EEPROM(&eeprom_contents, sizeof(eeprom_contents));
#endif

uint8_t rf_id;
uint8_t rf_upstream;

uint32_t battery_voltage;

RFLink *rflink = NULL;

SerialCLI CLI(SerialUSB);

ADCRead adcread;
int16_t core_temperature;
SDLogging logging(SD_CS_PIN, SD_CD_PIN);
GPRS gprs(GPRS_RST_PIN, GPRS_DTR_PIN, &logging);
Adafruit_FRAM_SPI fram(FRAM_CS_PIN);
SSD1306 oled;
LCDDeck lcdDeck(&oled, true);

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


class GetRFIDCLICommand : public CLICommand
{
    public:
        GetRFIDCLICommand(void) : CLICommand("get_rf_link", 0) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t rf_id = EEPROM.read(EEPROM_OFFSET(rf_link_id));
                serial()->print("Current RF ID = ");
                serial()->println(rf_id, HEX);
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
                serial()->print("New RF ID = ");
                serial()->println(rf_id, HEX);
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
                serial()->print("Current RF Upstream = ");
                serial()->println(rf_up, HEX);
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
                serial()->print("New RF Upstream = ");
                serial()->println(rf_up, HEX);
                return 1;
            };
};


// TODO:  CLI commands for:
//            get/set APN
//            get/set URL (from SHA204)
//            get/set AWS Creds (from SHA204)
//            enable/disable GPRS

void setup() 
{
    CLI.registerCommand(new GetRFIDCLICommand());
    CLI.registerCommand(new SetRFIDCLICommand());
    CLI.registerCommand(new GetRFUpstreamCLICommand());
    CLI.registerCommand(new SetRFUpstreamCLICommand());

    CLI.initialize();

    rf_id = EEPROM.read(EEPROM_OFFSET(rf_link_id));
    rf_upstream = EEPROM.read(EEPROM_OFFSET(rf_link_upstream));

    bool framInit = fram.begin();
    if (!framInit) {
        CLI.serial()->println("Can't find attached FRAM");
    }
    
    oled.begin(SSD1306_SWITCHCAPVCC);
    if (framInit) {
        oled.attachRAM(&fram, 0x0000, 0x0400);
        gprs.attachRAM(&fram);
    }
    oled.display();

    lcdDeck.addFrame(new LCDScreen("Core Temp",
                     (void *)&core_temperature, formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Battery",
                     (void *)&battery_voltage, formatAutoScale, "V"));

    lcdTicks = 0;

    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id, rf_upstream);

    Sha204Initialize();
}

void loop() 
{
    uint8_t *buffer = NULL;
    uint8_t len = 0;
    static bool gprsDisabled;

    CLI.handleInput();

    lcdTicks++;
    if (lcdTicks >= SWAP_COUNT) {
        lcdTicks -= SWAP_COUNT;

        core_temperature = adcread.readCoreTemperature();
        // Note:  actual max voltage is 3.3V - 0.6V = 2.7V
        // We have an external divider to take 4.2V -> 2.1V, the default
        // reference includes a gain of /2, so a full battery reading
        // should end up at 1.05V when measured, an ADC reading of 2606
        // (0xA2E)
        battery_voltage = adcread.mapPin(VBATT_ADC_PIN, 0, 6600);

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
    len = rflink->receive(rf_rx_buffer, RF_RX_BUFFER_SIZE, &source);
    if (len) {
        gprs.sendCborPacket(source, rf_rx_buffer, len);
    }

    gprs.stateMachine();

//    LowPower.idle(SLEEP_1S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
//                  SPI_OFF, USART0_ON, TWI_OFF);
    delay(LOOP_CADENCE);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
