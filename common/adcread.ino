#include "adcread.h"

uint16_t adc_readings[8];	///< raw readings
int16_t core_temperature;	///< in 1/10 degree C
uint16_t vcc;			///< in millivolts

uint16_t readVcc(void)
{
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  uint32_t result = (high << 8) | low;

  // result = (1100mV / Vcc) * 1023
  // we want Vcc
  // Vcc = 1100mV * 1023 / result
  // Vcc = 1125300mV / result
  result = 1125300L / result;
  return (uint16_t)result; // Vcc in millivolts
}

int16_t readAvrTemperature(void)
{
  // set the reference to 1.1V and the measurement to the internal temperature sensor
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  int32_t result = (high << 8) | low;

  // 242mV (225 measured) = -45C, 380mV (353 measured) = 85C
  // use linear regression to convert from measured value to celcius
  result -= 225;
  result *= 1200; // * 120, have 0.1 deg
  result >>= 7;   // /= 128
  result -= 450;
  return (int16_t)result;
}

uint32_t convertVoltage(uint8_t input, uint16_t factor)
{
  // reading is from ADC (0 - 1023)
  // factor is 1000/gain - gain is Vcc / max voltage in the circuit
  // return value is measured in millivolts

  // reading = (voltage * gain) / Vcc * 1023
  // reading = voltage * 1000 / factor / Vcc * 1023
  // voltage = reading * factor * Vcc / 1000 / 1023
  // voltage = ((reading * Vcc / 1023) * factor) / 1000
  uint32_t value = (uint32_t)analogRead(input);
  value *= vcc;
  value /= 1023;
  value *= factor;
  value /= 1000;
  return value;
}

uint32_t convertCurrent(uint8_t input, uint16_t factor1, uint16_t factor2)
{
  // reading is from ADC (0 - 1023)
  // factor1 is factor2 * 1000 / gain - gain is Vcc / max current in the circuit
  // return value is measured in microamps

  // reading = (current * gain) / Vcc * 1023
  // reading = current * 1000 * factor2 / factor1 / Vcc * 1023
  // current = reading * factor1 * Vcc / factor2 / 1000 / 1023
  // current = (((reading * Vcc / 1023) * factor1) / factor2) / 1000
  // to get current in microamps.
  // current = ((reading * Vcc / 1023) * factor1) / factor2
  uint32_t value = (uint32_t)analogRead(input);
  value *= vcc;
  value /= 1023;
  value *= factor1;
  value /= factor2;
  return value;
}

uint32_t calculatePower(uint32_t voltage, uint32_t current)
{
  // voltage in millivolts
  // current in microamps
  // result in milliwatts

  current >>= 6;  // We need to lose some bits to fit the calculation into 32 bits along the way
  uint32_t result = voltage * current;
  result /= 15625;  // divide result (in nanowatts down to milliwatts - factor of 1e6 = (15625 << 6)
  return result;
}

void ADCPoll(void)
{
  vcc = readVcc();
  core_temperature = readAvrTemperature();

  // Setup to read ADC readings with VCC reference
  analogReference(DEFAULT);

  for (int i = 0; i < 8; i++) {
    adc_readings[i] = (uint16_t)analogRead(i);
  }
}

// vim:ts=4:sw=4:ai:et:si:sts=4
