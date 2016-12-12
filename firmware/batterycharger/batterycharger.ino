#include <avr/sleep.h>
#include <SPI.h>

#include <SdFat.h>

#include "rflink.h"
#include "rtclock.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "pwm.h"
#include "cbormap.h"
#include "sdlogging.h"
#include "serialcli.h"
#include "battery.h"

#include <Adafruit_GFX.h>
#include <gfxfont.h>

#include <Adafruit_ILI9340.h>

// in ms
#define LOOP_CADENCE 100

SdFat sdcard;

#define RF_CS_PIN 4
#define RF_CE_PIN 5
#define RF_IRQ_PIN 2

#define SD_CS_PIN 20

#define LIGHT_ADC_PIN 7

#define TEST_VIN   0
#define TEST_BATT1 1
#define TEST_BATT2 2
#define TEST_LIION 3
#define TEST_3V3   4
#define TEST_5V    5

PowerMonitor *monitors[6];
uint16_t voltages[6];
uint32_t currents[6];
uint32_t powers[6];
uint16_t light;

static const char temp_string[] = "Temp";
static const char *line_string[6] = {
    "IN", "BATT1", "BATT2", "LiIon", "+3V3", "+5V"
};

RFLink *rflink = NULL;
SleepTimer sleepTimer(LOOP_CADENCE);

void CborMessageBuildLocal(void);

void CborMessageBuildLocal(void)
{
    DateTime now = RTClockGetTime();
    CborMessageInitialize();
    CborMessageAddMap(7);
    CborMapAddTimestamp(now.year(), now.month(), now.day(), now.hour(),
                        now.minute(), now.second());
    CborMapAddInteger(CBOR_KEY_SOURCE, CBOR_SOURCE_CHARGER);
    CborMapAddInteger(CBOR_KEY_RF_ID, 0xFE);
    CborMapAddArray(CBOR_KEY_VOLTAGE_ARRAY, voltages, 4);
    CborMapAddArray(CBOR_KEY_CURRENT_ARRAY, currents, 4);
    CborMapAddArray(CBOR_KEY_POWER_ARRAY, powers, 4);
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
                    Serial.print("Battery ");
                    Serial.print((char *)args[0]);
                    Serial.println(" invalid");
                    return 0;
                }
                int8_t chargerEnable = *args[1] - 0x31;
                if (chargerEnable < 0 || chargerEnable > 1) {
                    Serial.print("Enabled value ");
                    Serial.print((char *)args[1]);
                    Serial.println(" invalid");
                    return 0;
                }

                battery[batteryNum].setEnabled(chargerEnable);

                Serial.print("Battery Charger ");
                Serial.print(batteryNum);
                if (chargerEnable) {
                    Serial.println(" enabled");
                } else {
                    Serial.println(" disabled");
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
                    Serial.print("Battery ");
                    Serial.print((char *)args[0]);
                    Serial.println(" invalid");
                    return 0;
                }
                int8_t desulfateEnable = *args[1] - 0x31;
                if (desulfateEnable < 0 || desulfateEnable > 1) {
                    Serial.print("Enabled value ");
                    Serial.print((char *)args[1]);
                    Serial.println(" invalid");
                    return 0;
                }

                battery[batteryNum].setDesulfate(desulfateEnable);

                Serial.print("Desulfator for battery ");
                Serial.print(batteryNum);
                if (desulfateEnable) {
                    Serial.println(" enabled");
                } else {
                    Serial.println(" disabled");
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
                    Serial.print("Battery ");
                    Serial.print((char *)args[0]);
                    Serial.println(" invalid");
                    return 0;
                }
                int8_t capacity = (int8_t)strtoul(args[1], 0, 10);
                if (capacity != 9 && capacity != 20) {
                    Serial.print("Enabled value ");
                    Serial.print((char *)args[1]);
                    Serial.println(" invalid");
                    return 0;
                }
                
                battery[batteryNum].setCapacity(capacity);

                Serial.print("Capacity of battery ");
                Serial.print(batteryNum);
                Serial.print(" set to ");
                Serial.print(capacity);
                Serial.println(" Ah");
                return 1;
            };
};


void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;

    monitors[0] = new INA219PowerMonitor(0x40, 18, 100, 10, 40.0);
    monitors[1] = new INA219PowerMonitor(0x41, 16, 100, 10, 5.0);
    monitors[2] = new INA219PowerMonitor(0x42, 16, 100, 10, 5.0);
    monitors[3] = new INA219PowerMonitor(0x43, 6, 10, 10, 5.0);
    monitors[4] = new INA219PowerMonitor(0x44, 6, 10, 10, 0.5);
    monitors[5] = new INA219PowerMonitor(0x45, 6, 10, 10, 3.0);
    
    Serial.begin(115200);

    cli.registerCommand(new BatteryCLICommand());
    cli.registerCommand(new DesulfateCLICommand());
    cli.registerCommand(new CapacityCLICommand());

    cli.initialize();

    SDCardInitialize(SD_CS_PIN);
    LcdInitialize();
    LcdClear();
    ScreenInitialize();
    ScreenRefresh();
    PWMInitialize(0, 0, OCR0A);
    RTClockInitialize();
    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, 0xFE);
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    noInterrupts();
    sleepTimer.enable();

    for (i = 0; i < 6; i++) {
        if (monitor[i]->readMonitor()) {
            voltages[i] = monitor[i]->voltage();
            currents[i] = monitor[i]->current();
            powers[i]   = monitor[i]->power();
        }
    }

    light = analogRead(LIGHT_ADC_PIN);
    PWMUpdateLed(light);
    updateScreenStrings();
    ScreenRefresh()

    RTClockPoll();
    CborMessageBuildLocal();
    CborMessageBuffer(&buffer, &len);
    if (buffer && len) {
        SDCardWrite(buffer, len);
    }

    battery[0].updateState();
    battery[1].updateState();
    cli.handleInput();

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
