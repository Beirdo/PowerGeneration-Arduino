#include <avr/sleep.h>
#include <string.h>
#include "nokialcd.h"

#define CLOCK_FREQUENCY 8000000

// 0 - PD0 - RXD - RXD
// 1 - PD1 - TXD - TXD
// 2 - PD2 - IO - unused
// 3 - PD3 - OC2B - PWM_LED
// 4 - PD4 - IO - unused
// 5 - PD5 - OC0B - PWM_CONV2
// 6 - PD6 - OC0A - PWM_CONV1
// 7 - PD7 - IO - unused

// 8 - PB0 - IO - unused
// 9 - PB1 - IO - LCD_DC
// 10 - PB2 - SS - SS
// 11 - PB3 - MOSI - MOSI
// 12 - PB4 - MISO - MISO
// 13 - PB5 - SCK - SCK

// 14/A0 - PC0 - ADC0 - ADC_I_5V
// 15/A1 - PC1 - ADC1 - ADC_V_VIN
// 16/A2 - PC2 - ADC2 - ADC_I_VIN
// 17/A3 - PC3 - ADC3 - ADC_V_VMPP
// 18/A4 - PC4 - ADC4 - ADC_I_VMPP
// 19/A5 - PC5 - ADC5 - ADC_V_15V

// 20 - PB6 - IO - unused
// 21 - IO - LCD_RST

// 22 - PC6 - RESET - RESET

// A6 - ADC6 - ADC_I_15V
// A7 - ADC7 - ADC_LIGHT


static const char temp_string[] = "Temp";
static const char *line_string[4] = {
  "+5V", "IN", "OUT", "+15V"
};

int readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  // result = (1100mV / Vcc) * 1023
  // we want Vcc
  // Vcc = 1100mV * 1023 / result
  // Vcc = 1125300mV / result
  result = 1125300L / result;
  return (int)result; // Vcc in millivolts
}

int readAvrTemperature(void) {
  // set the reference to 1.1V and the measurement to the internal temperature sensor
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  int result = (high << 8) | low;

  // 242mV (225 measured) = -45C, 380mV (353 measured) = 85C
  // use linear regression to convert from measured value to celcius
  result -= 225;
  result *= 1200; // * 120, have 0.1 deg
  result >>= 7;   // /= 128
  result -= 450;
  return result;
}


long convertVoltage(char input, int vcc, int factor)
{
  // reading is from ADC (0 - 1023)
  // vcc is measured in millivolts
  // factor is 1000/gain - gain is Vcc / max voltage in the circuit
  // return value is measured in millivolts

  // reading = (voltage * gain) / Vcc * 1023
  // reading = voltage * 1000 / factor / Vcc * 1023
  // voltage = reading * factor * Vcc / 1000 / 1023
  // voltage = ((reading * Vcc / 1023) * factor) / 1000
  long value = (long)analogRead(input);
  value *= vcc;
  value /= 1023;
  value *= factor;
  value /= 1000;
  return value;
}

long convertCurrent(char input, int vcc, int factor1, int factor2)
{
  // reading is from ADC (0 - 1023)
  // vcc is measured in millivolts
  // factor1 is factor2 * 1000 / gain - gain is Vcc / max current in the circuit
  // return value is measured in microamps

  // reading = (current * gain) / Vcc * 1023
  // reading = current * 1000 * factor2 / factor1 / Vcc * 1023
  // current = reading * factor1 * Vcc / factor2 / 1000 / 1023
  // current = (((reading * Vcc / 1023) * factor1) / factor2) / 1000
  // to get current in microamps.
  // current = ((reading * Vcc / 1023) * factor1) / factor2
  long value = (long)analogRead(input);
  value *= vcc;
  value /= 1023;
  value *= factor1;
  value /= factor2;
  return value;
}

#define TEST_5V   0
#define TEST_VIN  1
#define TEST_MPPT 2
#define TEST_15V  3

int readings[8];
long voltages[4];
long currents[4];
long powers[4];
int light;
int temperature;

long prev_v_in;
long prev_i_in;

long calculatePower(long voltage, long current)
{
  // voltage in millivolts
  // current in microamps
  // result in milliwatts

  current >>= 6;  // We need to lose some bits to fit the calculation into 32 bits along the way
  long result = voltage * current;
  result /= 15625;  // divide result (in nanowatts down to milliwatts - factor of 1e6 = (15625 << 6)
  return result;
}

void ADCPoll(void)
{
  prev_v_in = voltages[TEST_VIN];
  prev_i_in = currents[TEST_VIN];

  int vcc = readVcc();
  voltages[TEST_5V] = (long)vcc;

  temperature = readAvrTemperature();

  // Setup to read ADC readings with VCC reference
  analogReference(DEFAULT);

  for (int i = 0; i < 8; i++) {
    readings[i] = analogRead(i);
  }

  currents[TEST_5V] = convertCurrent(readings[0], vcc, 55555, 1000);

  voltages[TEST_VIN] = convertVoltage(readings[1], vcc, 24046);
  currents[TEST_VIN] = convertCurrent(readings[2], vcc, 2012, 1);

  voltages[TEST_MPPT] = convertVoltage(readings[3], vcc, 24046);
  currents[TEST_MPPT] = convertCurrent(readings[4], vcc, 2012, 1);

  voltages[TEST_15V] = convertVoltage(readings[5], vcc, 3004);
  currents[TEST_15V] = convertCurrent(readings[6], vcc, 2000, 1);

  light = readings[7];

  for (int i = 0; i < 4; i++) {
    powers[i] = calculatePower(voltages[i], currents[i]);
  }
}

void PWMInitialize(void)
{
  // Set both OC0A and OC0B to Fast PWM mode, starting from BOTTOM, going to TOP, clearing output at OCRA
  TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
  // prescale of 1, gives a PWM frequency of 31.25kHz (off a 8MHz internal RC osc)
  TCCR0B = _BV(CS00);

  // Setup OC2B to Fast PWM mode too.  Prescaler of 64 for a PWM frequency of 488Hz (from 8MHz)
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS22);
}

uint8_t pwm_conv1 = 0x00;
uint8_t pwm_conv2 = 0x00;
uint8_t pwm_led = 0x00;

void updateLedPwm(void)
{
  uint8_t value = 0xFF - (uint8_t)(light >> 2);
  pwm_led = value;

  OCR2B = pwm_led;
}

#define MPPT_INTERVAL 1
#define MPPT_INCREMENT(x)  ((x) > (0xFF - MPPT_INTERVAL) ? 0xFF : (x) + MPPT_INTERVAL)
#define MPPT_DECREMENT(x)  ((x) < MPPT_INTERVAL ? 0x00 : (x) - MPPT_INTERVAL)

void mppt(void)
{
  // Incremental conductance
  long delta_v = voltages[TEST_VIN] - prev_v_in;
  long delta_i = currents[TEST_VIN] - prev_i_in;

  if (delta_v == 0) {
    if (delta_i == 0) {
      // No change, we are there
    } else if (delta_i > 0) {
      pwm_conv1 = MPPT_INCREMENT(pwm_conv1);
    } else {
      pwm_conv1 = MPPT_DECREMENT(pwm_conv1);
    }
  } else {
    long di_dv = delta_i / delta_v;
    long i_v = -1 * currents[TEST_VIN] / voltages[TEST_VIN];
    if (di_dv = i_v) {
      // No change, we are there
    } else if (di_dv > i_v) {
      pwm_conv1 = MPPT_INCREMENT(pwm_conv1);
    } else {
      pwm_conv1 = MPPT_DECREMENT(pwm_conv1);
    }
  }
  OCR0A = pwm_conv1;
}

#define REGULATE_RIPPLE 120   // millivolts ripple allowed (peak)

void regulateOutput(void)
{
  int value = 15000 - (int)voltages[TEST_15V];

  // We want to regulate the 15V output to 15V (duh)
  if (value >= - REGULATE_RIPPLE && value <= REGULATE_RIPPLE) {
    // Do nothing, we are close enough
  } else {
    // Try using linear regression to get closer to the right spot
    int scale = pwm_conv2 * 1000000 / (int)voltages[TEST_15V];
    value *= scale;
    value /= 1000000;
    value += (int)pwm_conv2;
    value = (value < 0 ? 0 : (value > 255 ? 255 : value));
    pwm_conv2 = (uint8_t)value;
  }
  OCR0B = pwm_conv2;
}

// in ms
#define LOOP_CADENCE 10
#define LOOP_PRESCALER 8
#define LOOP_TICK_CLOCK (CLOCK_FREQUENCY / (2 * LOOP_PRESCALER))
#define LOOP_TICK_TOP (LOOP_CADENCE * LOOP_TICK_CLOCK / 1000)
#define HI_BYTE(x)  (((x) >> 8) & 0xFF)
#define LO_BYTE(x)  ((x) & 0xFF)
#define LOOP_CS_BITS  (LOOP_PRESCALER == 1 ? (_BV(CS10)) : (LOOP_PRESCALER == 8 ? (_BV(CS11)) : (LOOP_PRESCALER == 64 ? (_BV(CS11) | _BV(CS10)) : \
                       (LOOP_PRESCALER == 256 ? (_BV(CS12)) : (LOOP_PRESCALER == 1024 ? (_BV(CS12) | _BV(CS10)) : 0)))))
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

void TimerInitialize(void)
{
  // Use Timer1 as a delay timer to pace the main loop.  We want to run every x ms and sleep between
  // Prescale of 1 gives a maximum timer of 16.384ms
  // Prescale of 8 gives a maximum timer of 131.072ms
  // Prescale of 64 gives a maximum timer of 1.049s
  // Prescale of 256 gives a maximum timer of 4.194s
  // Prescale of 1024 gives a maximum timer of 16.777s
  TCCR1A = 0x00;
  TCCR1B = _BV(WGM12);  // Set it up in CTC mode, but leave it disabled
  TCCR1C = 0x00;
  OCR1AH = HI_BYTE(LOOP_TICK_TOP);
  OCR1AL = LO_BYTE(LOOP_TICK_TOP);
  TIMSK1 = _BV(OCIE1A);
}

void TimerEnable(void)
{
  // disable it
  TCCR1B = _BV(WGM12);

  // clear it
  TCNT1H = 0x00;
  TCNT1L = 0x00;

  // clear pending flag
  TIFR1 = _BV(OCF1A);

  // enable it
  TCCR1B = _BV(WGM12) | LOOP_CS_BITS;
}

void TimerDisable(void)
{
  TCCR1B = _BV(WGM12);
}

ISR(TIMER1_COMPA_vect)
{
  TimerDisable();
  swap_count++;
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
  if (swap_count >= SWAP_COUNT) {
    show_temperature = 1 - show_temperature;
    swap_count = 0;
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
    printValue(powers[TEST_5V], 2, &screen_lines[0][6], 5);
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
  printValue(powers[TEST_15V], 1000000, &screen_lines[5][6], 5);
  screen_lines[5][11] = 'W';
}

void setup(void)
{
  // Setup sleep mode to idle mode
  SMCR = 0x00;

  LcdInitialize();
  LcdClear();
  ScreenInitialize();
  ScreenRefresh();
  TimerInitialize();
  PWMInitialize();
}

void loop(void)
{
  noInterrupts();
  TimerEnable();
  ADCPoll();
  updateLedPwm();
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

