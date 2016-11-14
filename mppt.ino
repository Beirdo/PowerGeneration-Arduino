#include <SPI.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <string.h>

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


#define PIN_SCE   10
#define PIN_RESET 21
#define PIN_DC    9
#define PIN_SDIN  12
#define PIN_SCLK  13

#define LCD_C     LOW
#define LCD_D     HIGH

#define LCD_X     84
#define LCD_Y     48

static const byte PROGMEM ASCII[][5] =
{
 {0x00, 0x00, 0x00, 0x00, 0x00} // 20  
,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c ¥
,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j 
,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e ←
,{0x78, 0x46, 0x41, 0x46, 0x78} // 7f →
};

char last_char;
char char_cache[5];

void LcdCharacter(char character)
{
  if (character < 0x20 || character > 0x7F) {
    character = 0x00;
  } else {
    character = character - 0x20;
  }
  
  if (character != last_char) {
    last_char = character;
    if (last_char == 0x00) {
      memset(char_cache, 0x00, 5);
    } else {
      for (int i = 0; i < 5; i++) {
        char_cache[i] = pgm_read_byte_near(&ASCII[last_char][i]);
      }
    }
  }
  
  LcdWrite(LCD_D, 0x00);
  for (int i = 0; i < 5; i++) {
    LcdWrite(LCD_D, char_cache[i]);
  }
  LcdWrite(LCD_D, 0x00);
}

void LcdClear(void)
{
  for (int i = 0; i < LCD_X * LCD_Y / 8; i++) {
    LcdWrite(LCD_D, 0x00);
  }
}

void LcdInitialize(void)
{
  pinMode(PIN_SCE, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  
  digitalWrite(PIN_RESET, LOW);
  digitalWrite(PIN_RESET, HIGH);

  last_char = 0x00;
  memset(char_cache, 0x00, 5);
  
  LcdWrite(LCD_C, 0x21);  // LCD Extended Commands.
  LcdWrite(LCD_C, 0xBF);  // Set LCD Vop (Contrast). //0xB1 
  LcdWrite(LCD_C, 0x04);  // Set Temp coefficent. // 0x04
  LcdWrite(LCD_C, 0x14);  // LCD bias mode 1:48. // 0x13
  LcdWrite(LCD_C, 0x0C);  // LCD in normal mode., 0x0D for inverse

  LcdWrite(LCD_C, 0x20);  // LCD Basic Commands
  LcdWrite(LCD_C, 0x0C);  // LCD in normal mode., 0x0D for inverse
}

void LcdString(char *characters)
{
  while (*characters)
  {
    LcdCharacter(*characters++);
  }
}

void LcdWrite(byte dc, byte data)
{
  digitalWrite(PIN_DC, dc);
  digitalWrite(PIN_SCE, LOW);
  SPI.transfer(data);
  digitalWrite(PIN_SCE, HIGH);
}

void gotoXY(int x, int y)
{
  LcdWrite(LCD_C, 0x80 | x);  // Column.
  LcdWrite(LCD_C, 0x40 | y);  // Row.  

}



void drawLine(void)
{
  unsigned char  j;  
   for(j=0; j<84; j++) // top
  {
          gotoXY (j,0);
    LcdWrite (LCD_D,0x01);
  }   
  for(j=0; j<84; j++) //Bottom
  {
          gotoXY (j,5);
    LcdWrite (LCD_D,0x80);
  }   

  for(j=0; j<6; j++) // Right
  {
          gotoXY (83,j);
    LcdWrite (LCD_D,0xff);
  }   
  for(j=0; j<6; j++) // Left
  {
          gotoXY (0,j);
    LcdWrite (LCD_D,0xff);
  }

}

#define COL_COUNT (LCD_X / 7)
#define ROW_COUNT (LCD_Y / 8)
typedef char screen_line_t[COL_COUNT];
screen_line_t screen_lines[ROW_COUNT];

void ScreenInitialize(void)
{
  memset(screen_lines, 0x00, COL_COUNT * ROW_COUNT);  
}

void ScreenRefresh(void)
{
  for (int i = 0; i < ROW_COUNT; i++) {
    gotoXY(0, i * 8);
    for (int j = 0; j < COL_COUNT; j++) {
      LcdCharacter(screen_lines[i][j]);
    }
  }
}

int readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  // result = (1100mV / Vcc) * 1023
  // we want Vcc
  // Vcc = 1100mV * 1023 / result
  // Vcc = 1125300mV / result
  result = 1125300L / result;
  return (int)result; // Vcc in millivolts
}

char readAvrTemperature(void) {
  // set the reference to 1.1V and the measurement to the internal temperature sensor
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  int result = (high<<8) | low;
  
  // 242mV (225 measured) = -45C, 380mV (353 measured) = 85C
  // use linear regression to convert from measured value to celcius
  result -= 225;
  result *= 120;
  result >>= 7;   // /= 128
  result -= 45;
  return (char)result;
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
char temperature;

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

ISR(TIMER1_COMPA_vect) {
  TimerDisable();
}

void setup(void)
{
  // Setup sleep mode to idle mode
  SMCR = 0x00;
  
  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

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

  // Go to sleep, get woken up by the timer
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
}

