#include <Arduino.h>
#include "adcread.h"

int16_t core_temperature;	///< in 1/10 degree C

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
  // use linear regression to convert from measured value to celcius * 10
  result = map(result, 225, 353, -450, 850);
  return (int16_t)result;
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

volatile bool INA219PowerMonitor::readCurrent(uint32_t &value)
{
    value = m_device.getCurrent_mA();
    return true;
}

volatile bool INA219PowerMonitor::readVoltage(uint32_t &value)
{
    value = m_device.getBusVoltage_mV();
    return true;
}

volatile bool INA219PowerMonitor::readPower(uint32_t &value)
{
    value = m_device.getPower_mA();
    return true;
}


ADS1115PowerMonitor::ADS1115PowerMonitor(uint8_t address, uint8_t current,
                                         uint8_t voltage, uint8_t currentPga,
                                         uint8_t voltagePga, float voltageGain,
                                         float currentGain) :
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
    value = convertPower();
    return true;
}


uint32_t ADS1115PowerMonitor::convertVoltage(void)
{
  // reading is from 16-bit signed ADC
  // gain value is the external gain
  //  i.e. 120V -> 3.3V is a gain of 0.0275
  uint32_t value = (uint32_t)(m_rawVoltage < 0 : -m_rawVoltage : m_rawVoltage);
  value *= m_device.getFullScale(m_voltageInput);
  value >>= 15;
  return (uint32_t)(float(value) / m_voltageGain);
}

uint32_t ADS1115PowerMonitor::convertCurrent(void)
{
  // reading is from 16-bit signed ADC
  // gain value is the external gain
  //  i.e. 10A -> 100mV is a gain of 0.01
  uint32_t value = (uint32_t)(m_rawCurrent < 0 : -m_rawCurrent : m_rawCurrent);
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
