#include <avr/sleep.h>
#include <SPI.h>

#include <SdFat.h>

#include "rflink.h"
#include "temperatures.h"
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

#define CLOCK_FREQUENCY 8000000

// in ms
#define LOOP_CADENCE 100

SdFat sdcard;

#define RF_CS_PIN 4
#define RF_CE_PIN 5

#define TEST_3V3   0
#define TEST_VIN   1
#define TEST_BATT1 2
#define TEST_BATT2 3

uint16_t temperatures[8];
uint16_t voltages[4];
uint32_t currents[4];
uint32_t powers[4];
uint16_t light;

static const char temp_string[] = "Temp";
static const char *line_string[4] = {
    "+3V3", "IN", "BATT1", "BATT2"
};

// TODO: implement I2C on port 1.  Wire only uses port 0

void convertADCReadings()
{
    voltages[TEST_3V3] = (long)vcc;
    currents[TEST_3V3] = convertCurrent(adc_readings[0], 64935, 80);

    voltages[TEST_VIN] = convertVoltage(adc_readings[1], 5412);
    currents[TEST_VIN] = convertCurrent(adc_readings[2], 60643, 20);

    voltages[TEST_BATT1] = convertVoltage(adc_readings[3], 4554);
    currents[TEST_BATT1] = convertCurrent(adc_readings[4], 60643, 20);

    voltages[TEST_BATT2] = convertVoltage(adc_readings[5], 4554);
    currents[TEST_BATT2] = convertCurrent(adc_readings[6], 60643, 20);

    light = adc_readings[7];

    for (int i = 0; i < 4; i++) {
        powers[i] = calculatePower(voltages[i], currents[i]);
    }
}
    
void CborMessageBuildLocal(void);

void CborMessageBuildLocal(void)
{
    DateTime now = RTClockGetTime();
    CborMessageInitialize();
    CborMessageAddMap(7);
    CborMapAddTimestamp(now.years(), now.months(), now.days(), now.hours(),
                        now.minutes(), now.seconds());
    CborMapAddSource(0);
    CborMapAddArray(CBOR_KEY_VOLTAGE_ARRAY, voltages, 4);
    CborMapAddArray(CBOR_KEY_CURRENT_ARRAY, currents, 4);
    CborMapAddArray(CBOR_KEY_POWER_ARRAY, powers, 4);
    CborMapAddArray(CBOR_KEY_TEMPERATURE_ARRAY, temperatures, 8);
    CborMapAddCoreTemperature(temperature);
}

void CborMessageBuildRemote(uint8_t source, uint8_t *payload, uint8_t len);

void CborMessageBuildRemote(uint8_t source, uint8_t *payload, uint8_t len)
{
    DateTime now = RTClockGetTime();
    CborMessageInitialize();
    CborMessageAddMap(3);
    CborMapAddTimestamp(now.years(), now.months(), now.days(), now.hours(),
                        now.minutes(), now.seconds());
    CborMapAddSource(source);
    CborMapAddCborPayload(payload, len);
}

class BatteryCLICommand(CLICommand)
{
    public:
        DisableCommand(void) : CLICommand("battery", 2);
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                int8_t batteryNum = *args[0] - 0x31;
                if (batteryNum < 0 || batteryNum > 1) {
                    Serial.print("Battery ");
                    Serial.print(args[0]);
                    Serial.println(" invalid");
                    return 0;
                }
                int8_t chargerEnable = *args[1] - 0x31;
                if (chargerEnable < 0 || chargerEnable > 1) {
                    Serial.print("Enabled value ");
                    Serial.print(args[1]);
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
}

class DesulfateCLICommand(CLICommand)
{
    public:
        DesulfateCLICommand(void) : CLICommand("desulfate", 2);
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                int8_t batteryNum = *args[0] - 0x31;
                if (batteryNum < 0 || batteryNum > 1) {
                    Serial.print("Battery ");
                    Serial.print(args[0]);
                    Serial.println(" invalid");
                    return 0;
                }
                int8_t desulfateEnable = *args[1] - 0x31;
                if (desulfateEnable < 0 || desulfateEnable > 1) {
                    Serial.print("Enabled value ");
                    Serial.print(args[1]);
                    Serial.println(" invalid");
                    return 0;
                }

                battery[batteryNum].setDesulfate(desulfateEnable);

                Serial.print("Desulfator for battery ");
                Serial.print(batteryNum);
                if (chargerEnable) {
                    Serial.println(" enabled");
                } else {
                    Serial.println(" disabled");
                }
                return 1;
            };
}

class CapacityCLICommand(CLICommand)
{
    public:
        CapacityCLICommand(void) : CLICommand("capacity", 2);
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                int8_t batteryNum = *args[0] - 0x31;
                if (batteryNum < 0 || batteryNum > 1) {
                    Serial.print("Battery ");
                    Serial.print(args[0]);
                    Serial.println(" invalid");
                    return 0;
                }
                int8_t capacity = (int8_t)strtoul(args[1], 0, 10);
                if (capacity != 9 && capacity != 20) {
                    Serial.print("Enabled value ");
                    Serial.print(args[1]);
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
}


void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;
    
    Serial.begin(115200);

    cli.registerCommand(BatteryCLICommand());
    cli.registerCommand(DesulfateCLICommand());
    cli.registerCommand(CapacityCLICommand());

    SDCardInitialize(20);
    LcdInitialize();
    LcdClear();
    ScreenInitialize();
    ScreenRefresh();
    TimerInitialize();
    PWMInitialize(0, 0, OCR0A);
    RTClockInitialize();
    TemperaturesInitialize();
    RFLinkInitialize(2, 0xFE);
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    noInterrupts();
    TimerEnable();
    ADCPoll();
    convertADCReadings();
    PWMUpdateLed(light);
    TemperaturesPoll(temperatures, 8);
    updateScreenStrings();
    ScreenRefresh()
    CborMessageBuildLocal();
    CborMessageBuffer(&buffer, &len);
    if (buffer && len) {
        SDCardWrite(buffer, len);
    }

    cli.handleInput();

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
