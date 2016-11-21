#include <avr/sleep.h>
#include <SPI.h>

#include <SdFat.h>

#include "rflink.h"
#include "temperatures.h"
#include "rtclock.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "pwm.h"

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

void setup() 
{
  // Setup sleep to idle mode
  SMCR = 0x00;
  
  Serial.begin(115200);

  LcdInitialize();
  LcdClear();
  ScreenInitialize();
  ScreenRefresh();
  TimerInitialize();
  PWMInitialize(0, 0, OCR0A);
  RTClockInitialize();
  TemperaturesInitialize();
  RFLinkInitialize(2, 0xFF);
}

void loop() 
{
  noInterrupts();
  TimerEnable();
  ADCPoll();
  convertADCReadings();
  PWMUpdateLed(light);
  TemperaturesPoll(temperatures, 8);
  updateScreenStrings();
  ScreenRefresh()

  // Go to sleep, get woken up by the timer
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
}

void blah(void)
{
  DateTime now = RTClockGetTime();
  delay(10000);
}
