#include <Arduino.h>
#include "adcread.h"

int16_t core_temperature;	///< in 1/10 degree C

inline int16_t readAdc(void);

uint16_t readVcc(void)
{
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  ADCSRA |= _BV(ADEN); // Enable ADC

  delay(20); // Wait for Vref to settle
  readAdc(); // Toss the first reading

  uint32_t result = (uint32_t)readAdc();

  // result = (1100mV / Vcc) * 1024
  // we want Vcc
  // Vcc = 1100mV * 1024 / result
  // Vcc = 1126400mV / result
  result = 1126400L / result;
  return (uint16_t)result; // Vcc in millivolts
}

int16_t readAvrTemperature(void)
{
  // set the reference to 1.1V and the measurement to the internal temperature sensor
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  ADCSRA |= _BV(ADEN); // Enable ADC

  delay(20); // Wait for Vref to settle
  readAdc(); // Toss the first reading

  int32_t avgTemp = 0;

  for (uint16_t i = 0; i < 1000; i++) {
    avgTemp += readAdc();
  }

  avgTemp -= 335200;  // offset of 335.2
  int16_t result = (int16_t)((float)avgTemp / 106.154);

  return result;
}

inline int16_t readAdc(void)
{
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  int32_t result = (int32_t)ADCW;
  return result;
}


PowerMonitor::PowerMonitor(void)
{
    m_current = 0;
    m_voltage = 0;
    m_power = 0;
}

bool PowerMonitor::readMonitor(void)
{
    uint32_t current;
    uint32_t voltage;
    uint32_t power;
    bool result;

    result = readCurrent(current);
    if (result) {
        result = readVoltage(voltage);
    }
    if (result) {
        result = readPower(power);
    }
    if (result) {
        m_current = current;
        m_voltage = voltage;
        m_power = power;
    }
    return result;
}


INA219PowerMonitor::INA219PowerMonitor(uint8_t address, uint8_t maxVoltage,
                                       uint16_t maxVShunt, uint16_t rShunt,
                                       float maxIExpected) :
        PowerMonitor(), m_device(INA219(address))
{
    m_address = address;

    m_device.begin(m_address);
    m_device.setCalibration(maxVoltage, maxVShunt, rShunt, maxIExpected);
}

bool INA219PowerMonitor::readCurrent(uint32_t &value)
{
    value = m_device.getCurrent_mA();
    return true;
}

bool INA219PowerMonitor::readVoltage(uint32_t &value)
{
    value = m_device.getBusVoltage_mV();
    return true;
}

bool INA219PowerMonitor::readPower(uint32_t &value)
{
    value = m_device.getPower_mW();
    return true;
}


ADS1115PowerMonitor::ADS1115PowerMonitor(uint8_t address,
        uint8_t current, uint8_t voltage,
        uint8_t currentPga, uint8_t voltagePga,
        float currentGain, float voltageGain) :
        PowerMonitor(), m_device(ADS1115(address))
{
    m_address = address;
    m_currentInput = current;
    m_voltageInput = voltage;
    m_currentPga = currentPga;
    m_voltagePga = voltagePga;
    m_currentGain = currentGain;
    m_voltageGain = voltageGain;
    m_rawCurrent = 0;
    m_rawVoltage = 0;
    m_lastCurrent = 0;
    m_lastVoltage = 0;

    m_device.initialize();
}

bool ADS1115PowerMonitor::readCurrent(uint32_t &value)
{
    m_device.setGain(m_currentPga);
    m_device.setMultiplexer(m_currentInput);
    m_rawCurrent = m_device.getConversion(true);
    m_lastCurrent = convertCurrent();
    value = m_lastCurrent;
    return true;
}

bool ADS1115PowerMonitor::readVoltage(uint32_t &value)
{
    m_device.setGain(m_voltagePga);
    m_device.setMultiplexer(m_voltageInput);
    m_rawVoltage = m_device.getConversion(true);
    m_lastVoltage = convertVoltage();
    value = m_lastVoltage;
    return true;
}

bool ADS1115PowerMonitor::readPower(uint32_t &value)
{
    value = calculatePower();
    return true;
}


uint32_t ADS1115PowerMonitor::convertVoltage(void)
{
  // reading is from 16-bit signed ADC
  // gain value is the external gain
  //  i.e. 120V -> 3.3V is a gain of 0.0275
  uint32_t value = (uint32_t)(m_rawVoltage < 0 ? -m_rawVoltage : m_rawVoltage);
  value *= m_device.getFullScale(m_voltageInput);
  value >>= 15;
  return (uint32_t)(float(value) / m_voltageGain);
}

uint32_t ADS1115PowerMonitor::convertCurrent(void)
{
  // reading is from 16-bit signed ADC
  // gain value is the external gain
  //  i.e. 10A -> 100mV is a gain of 0.01
  uint32_t value = (uint32_t)(m_rawCurrent < 0 ? -m_rawCurrent : m_rawCurrent);
  value *= m_device.getFullScale(m_currentInput);
  value >>= 15;
  return (uint32_t)(float(value) / m_currentGain);
}

uint32_t ADS1115PowerMonitor::calculatePower(void)
{
  // voltage in millivolts
  // current in milliamps
  // result in milliwatts

  // divide result (in microwatts) down to milliwatts
  // - factor of 1e3 = (250 << 2)
  uint32_t result = (m_lastVoltage >> 1) * (m_lastCurrent >> 1);
  result /= 250;
  return result;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
