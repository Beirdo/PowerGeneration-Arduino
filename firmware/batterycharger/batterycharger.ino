#include <SPI.h>
#include <LowPower.h>
#include <Adafruit_GFX.h>
#include <SSD1306.h>
#include <Adafruit_FRAM_SPI.h>

#include "rflink.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"
#include "battery.h"
#include "lcdscreen.h"
#include "eeprom.h"
#include "utils.h"

// in ms
#define LOOP_CADENCE 120
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

uint16_t lcdTicks;
int8_t lcdIndex;

#define RF_CE_PIN 5
#define RF_CS_PIN 10
#define RF_IRQ_PIN 2

#define FRAM_CS_PIN 7

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix SSD1306.h!");
#endif

SerialCLI CLI(Serial);

static const eeprom_t EEMEM eeprom_contents = { 0xFF, 0xFF };
uint8_t rf_id;
uint8_t rf_upstream;

#define TEST_VIN   0
#define TEST_BATT1 1
#define TEST_BATT2 2
#define TEST_LIION 3
#define TEST_3V3   4
#define TEST_5V    5

ADCRead adcread;
int16_t core_temperature;
PowerMonitor *monitors[6];
uint32_t voltages[6];
uint32_t currents[6];
uint32_t powers[6];

RFLink *rflink = NULL;

Adafruit_FRAM_SPI fram(FRAM_CS_PIN);
SSD1306 oled;
LCDDeck lcdDeck(&oled);

void CborMessageBuild(void);

void CborMessageBuild(void)
{
    CborMessageInitialize();
    CborMessageAddMap(6);
    CborMapAddInteger(CBOR_KEY_SOURCE, CBOR_SOURCE_CHARGER);
    CborMapAddInteger(CBOR_KEY_RF_ID, 0xFE);
    CborMapAddArray(CBOR_KEY_VOLTAGE_ARRAY, voltages, 6);
    CborMapAddArray(CBOR_KEY_CURRENT_ARRAY, currents, 6);
    CborMapAddArray(CBOR_KEY_POWER_ARRAY, powers, 6);
    CborMapAddInteger(CBOR_KEY_CORE_TEMPERATURE, core_temperature);
}

class BatteryCLICommand : public CLICommand
{
    public:
        BatteryCLICommand(void) : CLICommand("battery", 2) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                int8_t batteryNum = *args[0] - 0x31;
                if (batteryNum < 0 || batteryNum > 1) {
                    CLI.serial()->print("Battery ");
                    CLI.serial()->print((char *)args[0]);
                    CLI.serial()->println(" invalid");
                    return 0;
                }
                int8_t chargerEnable = *args[1] - 0x31;
                if (chargerEnable < 0 || chargerEnable > 1) {
                    CLI.serial()->print("Enabled value ");
                    CLI.serial()->print((char *)args[1]);
                    CLI.serial()->println(" invalid");
                    return 0;
                }

                battery[batteryNum].setEnabled(chargerEnable);

                CLI.serial()->print("Battery Charger ");
                CLI.serial()->print(batteryNum);
                if (chargerEnable) {
                    CLI.serial()->println(" enabled");
                } else {
                    CLI.serial()->println(" disabled");
                }
                return 1;
            };
};

class DesulfateCLICommand : public CLICommand
{
    public:
        DesulfateCLICommand(void) : CLICommand("desulfate", 2) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                int8_t batteryNum = *args[0] - 0x31;
                if (batteryNum < 0 || batteryNum > 1) {
                    CLI.serial()->print("Battery ");
                    CLI.serial()->print((char *)args[0]);
                    CLI.serial()->println(" invalid");
                    return 0;
                }
                int8_t desulfateEnable = *args[1] - 0x31;
                if (desulfateEnable < 0 || desulfateEnable > 1) {
                    CLI.serial()->print("Enabled value ");
                    CLI.serial()->print((char *)args[1]);
                    CLI.serial()->println(" invalid");
                    return 0;
                }

                battery[batteryNum].setDesulfate(desulfateEnable);

                CLI.serial()->print("Desulfator for battery ");
                CLI.serial()->print(batteryNum);
                if (desulfateEnable) {
                    CLI.serial()->println(" enabled");
                } else {
                    CLI.serial()->println(" disabled");
                }
                return 1;
            };
};

class CapacityCLICommand : public CLICommand
{
    public:
        CapacityCLICommand(void) : CLICommand("capacity", 2) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                int8_t batteryNum = *args[0] - 0x31;
                if (batteryNum < 0 || batteryNum > 1) {
                    CLI.serial()->print("Battery ");
                    CLI.serial()->print((char *)args[0]);
                    CLI.serial()->println(" invalid");
                    return 0;
                }
                int8_t capacity = (int8_t)strtoul(args[1], 0, 10);
                if (capacity != 9 && capacity != 20) {
                    CLI.serial()->print("Enabled value ");
                    CLI.serial()->print((char *)args[1]);
                    CLI.serial()->println(" invalid");
                    return 0;
                }
                
                battery[batteryNum].setCapacity(capacity);

                CLI.serial()->print("Capacity of battery ");
                CLI.serial()->print(batteryNum);
                CLI.serial()->print(" set to ");
                CLI.serial()->print(capacity);
                CLI.serial()->println(" Ah");
                return 1;
            };
};


class InitializeLogoCLICommand : public CLICommand
{
    public:
        InitializeLogoCLICommand(void) : CLICommand("initlogo", 0) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                CLI.serial()->println("Writing logo to FRAM");
                oled.initializeLogo();
                CLI.serial()->println("Done");
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
                CLI.serial()->print("Current RF ID = ");
                CLI.serial()->println(rf_id, HEX);
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
                CLI.serial()->print("New RF ID = ");
                CLI.serial()->println(rf_id, HEX);
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
                CLI.serial()->print("Current RF Upstream = ");
                CLI.serial()->println(rf_up, HEX);
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
                CLI.serial()->print("New RF Upstream = ");
                CLI.serial()->println(rf_up, HEX);
                return 1;
            };
};


void setup() 
{
    monitors[0] = new INA219PowerMonitor(0x40, 18, 100, 10, 40.0);
    monitors[1] = new INA219PowerMonitor(0x41, 16, 100, 10, 5.0);
    monitors[2] = new INA219PowerMonitor(0x42, 16, 100, 10, 5.0);
    monitors[3] = new INA219PowerMonitor(0x43, 6, 10, 10, 5.0);
    monitors[4] = new INA219PowerMonitor(0x44, 6, 10, 10, 0.5);
    monitors[5] = new INA219PowerMonitor(0x45, 6, 10, 10, 3.0);
    
    CLI.registerCommand(new GetRFIDCLICommand());
    CLI.registerCommand(new SetRFIDCLICommand());
    CLI.registerCommand(new GetRFUpstreamCLICommand());
    CLI.registerCommand(new SetRFUpstreamCLICommand());
    CLI.registerCommand(new BatteryCLICommand());
    CLI.registerCommand(new DesulfateCLICommand());
    CLI.registerCommand(new CapacityCLICommand());
    CLI.registerCommand(new InitializeLogoCLICommand());
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
    }
    oled.display();

    lcdDeck.addFrame(new LCDScreen("Core Temp",
                     (void *)&core_temperature, formatTemperature, "C"));

    lcdDeck.addFrame(new LCDScreen("Vin", (void *)&voltages[TEST_VIN],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Iin", (void *)&currents[TEST_VIN],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pin", (void *)&powers[TEST_VIN],
                     formatAutoScale, "W"));

    lcdDeck.addFrame(new LCDScreen("Vbatt1", (void *)&voltages[TEST_BATT1],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Ibatt1", (void *)&currents[TEST_BATT1],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pbatt1", (void *)&powers[TEST_BATT1],
                     formatAutoScale, "W"));

    lcdDeck.addFrame(new LCDScreen("Vbatt2", (void *)&voltages[TEST_BATT2],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Ibatt2", (void *)&currents[TEST_BATT2],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pbatt2", (void *)&powers[TEST_BATT2],
                     formatAutoScale, "W"));

    lcdDeck.addFrame(new LCDScreen("Vliion", (void *)&voltages[TEST_LIION],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Iliion", (void *)&currents[TEST_LIION],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pliion", (void *)&powers[TEST_LIION],
                     formatAutoScale, "W"));

    lcdDeck.addFrame(new LCDScreen("Vcc", (void *)&voltages[TEST_3V3],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Icc", (void *)&currents[TEST_3V3],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pcc", (void *)&powers[TEST_3V3],
                     formatAutoScale, "W"));

    lcdDeck.addFrame(new LCDScreen("Vdd", (void *)&voltages[TEST_5V],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Idd", (void *)&currents[TEST_5V],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pdd", (void *)&powers[TEST_5V],
                     formatAutoScale, "W"));

    lcdTicks = 0;

    BatteryChargerInitialize();
    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id, rf_upstream);
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    for (uint8_t i = 0; i < 6; i++) {
        if (monitors[i]->readMonitor()) {
            voltages[i] = monitors[i]->voltage();
            currents[i] = monitors[i]->current();
            powers[i]   = monitors[i]->power();
        }
    }

    updateAllIO();

    lcdTicks++;
    if (lcdTicks >= SWAP_COUNT) {
        lcdTicks -= SWAP_COUNT;

        core_temperature = adcread.readCoreTemperature();
        lcdIndex = lcdDeck.nextIndex();
        lcdDeck.formatFrame(lcdIndex);
        lcdDeck.displayFrame();

        CborMessageBuild();
        CborMessageBuffer(&buffer, &len);
        if (buffer && len) {
            rflink->send(buffer, len);
        }
    }

    CLI.handleInput();

    LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_ON,
                  SPI_OFF, USART0_ON, TWI_OFF);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
