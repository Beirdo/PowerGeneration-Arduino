#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <string.h>
#include <stdlib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "rflink.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "converterpwm.h"
#include "cbormap.h"
#include "serialcli.h"
#include "lcdscreen.h"

// in ms
#define LOOP_CADENCE 10
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

uint16_t lcdTicks;
int8_t lcdIndex;

#define RF_CE_PIN 25
#define RF_CS_PIN 4
#define RF_IRQ_PIN 2

#define CONV1_PWM OCR0A
#define CONV2_PWM OCR0B

#define OLED_RESET -1

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

const uint8_t EEMEM rf_link_id = 0;
uint8_t rf_id;

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
SleepTimer sleepTimer(LOOP_CADENCE);

Adafruit_SSD1306 LCD(OLED_RESET);
LCDDeck lcdDeck(&LCD);
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

void setup(void)
{
    // Setup sleep mode to idle mode
    SMCR = 0x00;

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

    cli.registerCommand(new EnableCLICommand());
    cli.registerCommand(new DisableCLICommand());
    cli.initialize();

    rf_id = EEPROM.read(rf_link_id);
    
    LCD.begin(SSD1306_SWITCHCAPVCC, 0x3D);
    LCD.display();

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

    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id);
}

void loop(void)
{
    uint8_t *buffer;
    uint8_t len;

    noInterrupts();
    sleepTimer.enable();

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
        if (lcdTicks >= SWAP_TIME) {
            lcdTicks -= SWAP_TIME;

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

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
