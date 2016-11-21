#include <EEPROM.h>
#include <avr/eeprom.h>

#include <avr/sleep.h>
#include <string.h>
#include "nokialcd.h"
#include "rflink.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "pwm.h"

#define CLOCK_FREQUENCY 8000000

// in ms
#define LOOP_CADENCE 10
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

#define RF_CE_PIN 8
#define RF_CS_PIN 20


static const uint8_t EEMEM rf_link_id = 0;

static const char temp_string[] = "Temp";
static const char *line_string[4] = {
  "+3V3", "IN", "MPPT", "+18V"
};

#define TEST_3V3  0
#define TEST_VIN  1
#define TEST_MPPT 2
#define TEST_18V  3

uint32_t voltages[4];
uint32_t currents[4];
uint32_t powers[4];
uint16_t light;

uint32_t prev_v_in = 0;
uint32_t prev_i_in = 0;

#define MPPT_INTERVAL 1
#define MPPT_INCREMENT(x)  ((x) > (0xFF - MPPT_INTERVAL) ? 0xFF : (x) + MPPT_INTERVAL)
#define MPPT_DECREMENT(x)  ((x) < MPPT_INTERVAL ? 0x00 : (x) - MPPT_INTERVAL)

void mppt(void)
{
  // Incremental conductance
  int32_t delta_v = (int32_t)voltages[TEST_VIN] - (int32_t)prev_v_in;
  int32_t delta_i = (int32_t)currents[TEST_VIN] - (int32_t)prev_i_in;

  if (delta_v == 0) {
    if (delta_i == 0) {
      // No change, we are there
    } else if (delta_i > 0) {
      pwm_conv1 = MPPT_INCREMENT(pwm_conv1);
    } else {
      pwm_conv1 = MPPT_DECREMENT(pwm_conv1);
    }
  } else {
    int32_t di_dv = delta_i / delta_v;
    int32_t i_v = -1 * currents[TEST_VIN] / voltages[TEST_VIN];
    if (di_dv = i_v) {
      // No change, we are there
    } else if (di_dv > i_v) {
      pwm_conv1 = MPPT_INCREMENT(pwm_conv1);
    } else {
      pwm_conv1 = MPPT_DECREMENT(pwm_conv1);
    }
  }
  PWMUpdateConverter(1, pwm_conv1);
}

#define REGULATE_RIPPLE 120   // millivolts ripple allowed (peak)

void regulateOutput(void)
{
  int16_t value = 18000 - (int)voltages[TEST_18V];

  // We want to regulate the 18V output to 18V (duh)
  if (value >= - REGULATE_RIPPLE && value <= REGULATE_RIPPLE) {
    // Do nothing, we are close enough
  } else {
    // Try using linear regression to get closer to the right spot
    int scale = pwm_conv2 * 1000000 / (int)voltages[TEST_18V];
    value *= scale;
    value /= 1000000;
    value += (int)pwm_conv2;
    value = (value < 0 ? 0 : (value > 255 ? 255 : value));
    pwm_conv2 = (uint8_t)value;
  }
  PWMUpdateConverter(2, pwm_conv2);
}

#define digit(x)  ((char)(((x) + 0x30) & 0xFF))
#define split_value(value, part, scale)  do { part = value % scale;  value = value / scale; } while(0)

void printTemperature(int value, char *buf, char maxlen)
{
  char index = maxlen - 1;

  // Temperatures don't need autoscaling
  char part;
  char sign = (value < 0 ? '-' : ' ');
  value = (value < 0 ? -value : value);

  split_value(value, part, 10);
  buf[index--] = digit(part);
  buf[index--] = '.';

  for (char i = 0; index >= 0 && (value != 0 || i == 0); i++) {
    split_value(value, part, 10);
    buf[index--] = digit(part);
  }

  buf[index] = sign;
}

void printValue(long value, char maxunits, char *buf, char maxlen)
{
  char index = maxlen - 1;

  // Power, voltage, current do need autoscaling
  if (value == 0) {
    buf[index--] = digit(0);
    return;
  }

  long tempdigit;
  char digitcount = 0;

  for (tempdigit = value; tempdigit; tempdigit /= 10) {
    digitcount++;
  }

  char units = maxunits - (digitcount / 3);;

  switch (units) {
    case 1:
      buf[index--] = 'm';
      break;
    case 2:
      buf[index--] = 'u';
      break;
    case 0:
      break;
    default:
      buf[index--] = digit(0);
      return;
  }

  char digits = digitcount % 3;
  digits = (digits ? digits : 3);

  char decimals = (index - 1 <= digits ? (index - digits) : 0);
  digits += decimals;

  long scale;
  for (scale = 1, digitcount = 0; digitcount < digits; scale *= 10) {
    digitcount++;
  }

  value /= scale;
  char newdigit;
  for (; digits > 0; digits--) {
    split_value(value, newdigit, 10);
    buf[index--] = digit(newdigit);
    if (decimals) {
      decimals--;
      if (!decimals) {
        buf[index--] = '.';
      }
    }
  }
}

void updateScreenStrings(void)
{
  if (timer_count >= SWAP_COUNT) {
    show_temperature = 1 - show_temperature;
    timer_count = 0;
  }

  for (int i = 0; i < 6; i++) {
    memset(screen_lines[i], 0x00, COL_COUNT);
  }

  if (show_temperature) {
    strcpy(screen_lines[0], temp_string);
    printTemperature(temperature, &screen_lines[0][5], 5);
    screen_lines[0][10] = 0x7F;  // °
    screen_lines[0][11] = 'C';
  } else {
    strcpy(screen_lines[0], line_string[0]);
    printValue(powers[TEST_3V3], 2, &screen_lines[0][6], 5);
    screen_lines[0][11] = 'W';
  }

  strcpy(screen_lines[1], line_string[1]);
  printValue(powers[TEST_VIN], 2, &screen_lines[1][6], 5);
  screen_lines[1][11] = 'W';

  printValue(voltages[TEST_VIN], 1, screen_lines[2], 5);
  screen_lines[2][5] = 'V';

  printValue(currents[TEST_VIN], 2, &screen_lines[2][7], 4);
  screen_lines[2][11] = 'A';

  strcpy(screen_lines[3], line_string[2]);
  printValue(powers[TEST_MPPT], 2, &screen_lines[3][6], 5);
  screen_lines[3][11] = 'W';

  printValue(voltages[TEST_MPPT], 1, screen_lines[4], 5);
  screen_lines[4][5] = 'V';

  printValue(currents[TEST_MPPT], 2, &screen_lines[4][7], 4);
  screen_lines[4][11] = 'A';

  strcpy(screen_lines[5], line_string[3]);
  printValue(powers[TEST_18V], 1000000, &screen_lines[5][6], 5);
  screen_lines[5][11] = 'W';
}

void convertADCReadings(void)
{
  voltages[TEST_3V3] = (long)vcc;
  currents[TEST_3V3] = convertCurrent(adc_readings[0], 65397, 395);

  voltages[TEST_VIN] = convertVoltage(adc_readings[1], 36494);
  currents[TEST_VIN] = convertCurrent(adc_readings[2], 62874, 21);

  voltages[TEST_MPPT] = convertVoltage(adc_readings[3], 36494);
  currents[TEST_MPPT] = convertCurrent(adc_readings[4], 62874, 21);

  voltages[TEST_18V] = convertVoltage(adc_readings[5], 5412);
  currents[TEST_18V] = convertCurrent(adc_readings[6], 63333, 19);

  light = adc_readings[7];

  for (int i = 0; i < 4; i++) {
    powers[i] = calculatePower(voltages[i], currents[i]);
  }
}

void setup(void)
{
  // Setup sleep mode to idle mode
  SMCR = 0x00;

  uint8_t rf_id = EEPROM.read(rf_link_id);
  
  LcdInitialize();
  LcdClear();
  ScreenInitialize();
  ScreenRefresh();
  TimerInitialize();
  PWMInitialize(OCR0A, OCR0B, OCR2B);
  RFLinkInitialize(7, rf_id);
}

void loop(void)
{
  noInterrupts();
  TimerEnable();

  prev_v_in = voltages[TEST_VIN];
  prev_i_in = currents[TEST_VIN];

  ADCPoll();
  convertADCReadings();
  PWMUpdateLed(light);
  mppt();
  regulateOutput();
  updateScreenStrings();
  ScreenRefresh();

  // Go to sleep, get woken up by the timer
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
}

