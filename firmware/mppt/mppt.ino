#include <EEPROM.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdlib.h>
#include <LowPower.h>
#include <Adafruit_GFX.h>
#include <SSD1306.h>
#include <Adafruit_FRAM_SPI.h>

#include "rflink.h"
#include "adcread.h"
#include "converterpwm.h"
#include "cbormap.h"
#include "serialcli.h"
#include "lcdscreen.h"
#include "eeprom.h"
#include "utils.h"

// in ms
#define LOOP_CADENCE 15
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

uint16_t lcdTicks;
int8_t lcdIndex;

#define RF_CE_PIN 25
#define RF_CS_PIN 4
#define RF_IRQ_PIN 2

#define FRAM_CS_PIN 7

#define CONV1_PWM OCR0A
#define CONV2_PWM OCR0B

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

static const eeprom_t EEMEM eeprom_contents = { 0xFF, 0xFF };
uint8_t rf_id;
uint8_t rf_upstream;

#define TEST_3V3  0
#define TEST_18V  1
#define TEST_VIN  2
#define TEST_MPPT 3

PowerMonitor *monitors[4];
uint32_t voltages[4];
uint32_t currents[4];
uint32_t powers[4];

uint32_t prev_v_in = 0;
uint32_t prev_i_in = 0;

uint8_t enabled = 1;

RFLink *rflink = NULL;

Adafruit_FRAM_SPI fram(FRAM_CS_PIN);
SSD1306 oled;
LCDDeck lcdDeck(&oled, true);
ConverterPWM mpptConverter(&CONV1_PWM);
ConverterPWM outConverter(&CONV2_PWM);

#define MPPT_INTERVAL 1

void mppt(void)
{
    // Incremental conductance
    int32_t delta_v = (int32_t)voltages[TEST_VIN] - (int32_t)prev_v_in;
    int32_t delta_i = (int32_t)currents[TEST_VIN] - (int32_t)prev_i_in;

    if (delta_v == 0) {
        if (delta_i == 0) {
            // No change, we are there
        } else if (delta_i > 0) {
            mpptConverter.increment(MPPT_INTERVAL);
        } else {
            mpptConverter.decrement(MPPT_INTERVAL);
        }
    } else {
        int32_t di_dv = delta_i / delta_v;
        int32_t i_v = -1 * currents[TEST_VIN] / voltages[TEST_VIN];
        if (di_dv = i_v) {
            // No change, we are there
        } else if (di_dv > i_v) {
            mpptConverter.increment(MPPT_INTERVAL);
        } else {
            mpptConverter.decrement(MPPT_INTERVAL);
        }
    }
}

#define REGULATE_RIPPLE 120   // millivolts ripple allowed (peak)

void regulateOutput(void)
{
    int32_t value = 18000L - (int32_t)voltages[TEST_18V];

    // We want to regulate the 18V output to 18V (duh)
    if (value >= - REGULATE_RIPPLE && value <= REGULATE_RIPPLE) {
        // Do nothing, we are close enough
    } else {
        // Try using linear regression to get closer to the right spot
        int32_t scale = outConverter.getValue() * 1000000L /
                        (int32_t)voltages[TEST_18V];
        value *= scale;
        value /= 1000000;
        value += (int32_t)outConverter.getValue();
        outConverter.updateValue((uint8_t)constrain(value, 0, 255));
    }
}

void CborMessageBuild(void);

void CborMessageBuild(void)
{
    CborMessageInitialize();
    CborMessageAddMap(5);
    CborMapAddInteger(CBOR_KEY_SOURCE, CBOR_SOURCE_MPPT);
    CborMapAddArray(CBOR_KEY_VOLTAGE_ARRAY, voltages, 4);
    CborMapAddArray(CBOR_KEY_CURRENT_ARRAY, currents, 4);
    CborMapAddArray(CBOR_KEY_POWER_ARRAY, powers, 4);
    CborMapAddInteger(CBOR_KEY_CORE_TEMPERATURE, core_temperature);
}

class EnableCLICommand : public CLICommand
{
    public:
        EnableCLICommand(void) : CLICommand("enable", 0) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                if (!enabled) {
                    prev_v_in = 0;
                    prev_i_in = 0;
                    enabled = 1;
                }
                Serial.println("Regulators enabled");
                return 1;
            };
};

class DisableCLICommand : public CLICommand
{
    public:
        DisableCLICommand(void) : CLICommand("disable", 0) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                enabled = 0;
                Serial.println("Regulators disabled");
                return 1;
            };
};

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
                uint8_t rf_id = atou8(args[0]);
                EEPROM.update(EEPROM_OFFSET(rf_link_id), rf_id);
                Serial.print("New RF ID = ");
                Serial.println(rf_id, HEX);
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
                Serial.print("Current RF Upstream = ");
                Serial.println(rf_up, HEX);
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
                Serial.print("New RF Upstream = ");
                Serial.println(rf_up, HEX);
                return 1;
            };
};


void setup(void)
{
    monitors[0] = new INA219PowerMonitor(0x40, 5, 30, 60, 0.540);
    monitors[1] = new INA219PowerMonitor(0x41, 18, 60, 6, 10.0);
    monitors[2] = new ADS1115PowerMonitor(0x48, ADS1115_MUX_P0_NG,
                                          ADS1115_MUX_P2_NG,
                                          ADS1115_PGA_0P256,
                                          ADS1115_PGA_4P096,
                                          0.01, 0.0274);
    monitors[3] = new ADS1115PowerMonitor(0x48, ADS1115_MUX_P1_NG,
                                          ADS1115_MUX_P3_NG,
                                          ADS1115_PGA_0P256,
                                          ADS1115_PGA_4P096,
                                          0.01, 0.0274);

    Serial.begin(115200);

    cli.registerCommand(new GetRFIDCLICommand());
    cli.registerCommand(new SetRFIDCLICommand());
    cli.registerCommand(new GetRFUpstreamCLICommand());
    cli.registerCommand(new SetRFUpstreamCLICommand());
    cli.registerCommand(new EnableCLICommand());
    cli.registerCommand(new DisableCLICommand());
    cli.registerCommand(new InitializeLogoCLICommand());
    cli.initialize();

    rf_id = EEPROM.read(EEPROM_OFFSET(rf_link_id));
    rf_upstream = EEPROM.read(EEPROM_OFFSET(rf_link_upstream));

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

    lcdDeck.addFrame(new LCDScreen("Vin", (void *)&voltages[TEST_VIN],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Iin", (void *)&currents[TEST_VIN],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pin", (void *)&powers[TEST_VIN],
                     formatAutoScale, "W"));

    lcdDeck.addFrame(new LCDScreen("Vmppt", (void *)&voltages[TEST_MPPT],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Imppt", (void *)&currents[TEST_MPPT],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pmppt", (void *)&powers[TEST_MPPT],
                     formatAutoScale, "W"));

    lcdDeck.addFrame(new LCDScreen("Vout", (void *)&voltages[TEST_18V],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Iout", (void *)&currents[TEST_18V],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pout", (void *)&powers[TEST_18V],
                     formatAutoScale, "W"));

    lcdDeck.addFrame(new LCDScreen("Vcc", (void *)&voltages[TEST_3V3],
                     formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Icc", (void *)&currents[TEST_3V3],
                     formatAutoScale, "A"));
    lcdDeck.addFrame(new LCDScreen("Pcc", (void *)&powers[TEST_3V3],
                     formatAutoScale, "W"));

    lcdTicks = 0;

    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id, rf_upstream);
}

void loop(void)
{
    uint8_t *buffer;
    uint8_t len;

    if (enabled) {
        prev_v_in = voltages[TEST_VIN];
        prev_i_in = currents[TEST_VIN];

        for (uint8_t i = 0; i < 4; i++) {
            if (monitors[i]->readMonitor()) {
                voltages[i] = monitors[i]->voltage();
                currents[i] = monitors[i]->current();
                powers[i]   = monitors[i]->power();
            }
        }

        mppt();
        regulateOutput();

        lcdTicks++;
        if (lcdTicks >= SWAP_COUNT) {
            lcdTicks -= SWAP_COUNT;

            lcdIndex = lcdDeck.nextIndex();
            lcdDeck.formatFrame(lcdIndex);
            lcdDeck.displayFrame();

            CborMessageBuild();
            CborMessageBuffer(&buffer, &len);
            if (buffer && len) {
                rflink->send(buffer, len);
            }
        }
    }

    cli.handleInput();

    LowPower.idle(SLEEP_15MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_ON,
                  SPI_OFF, USART0_ON, TWI_OFF);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
