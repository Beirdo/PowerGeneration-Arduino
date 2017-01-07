#include <Arduino.h>
#include "adcread.h"

#ifdef __AVR__
ADCReadAVR::ADCReadAVR() : ADCReadBase()
{
}

uint16_t ADCReadAVR::readVcc(void)
{
    // Read 1.1V reference against AVcc
    // set the reference to Vcc and the measurement to the internal 1.1V
    // reference
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    ADCSRA |= _BV(ADEN); // Enable ADC

    delay(20); // Wait for Vref to settle
    readAdc(); // Toss the first reading

    uint32_t result = (uint32_t)readAdc();

    // result = (1100 [mV] / Vcc [mV]) * 1024
    // we want Vcc [mV]
    // Vcc = 1100 [mV] * 1024 / result
    // Vcc = 1126400 [mV] / result
    result = 1126400L / result;
    return (uint16_t)result; // Vcc in millivolts
}

int16_t ADCReadAVR::readCoreTemperature(void)
{
    // set the reference to 1.1V and the measurement to the internal
    // temperature sensor
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

    analogReference(DEFAULT);

    return result;
}

int32_t ADCReadAVR::mapPin(uint8_t pin, int32_t low, int32_t high)
{
    analogReference(DEFAULT);

    int32_t value = map(analogRead(pin), 0, 1023, low, high);
    return value;
}

inline int16_t ADCReadAVR::readAdc(void)
{
    ADCSRA |= _BV(ADSC); // Start conversion
    while (bit_is_set(ADCSRA, ADSC)); // measuring

    int32_t result = (int32_t)ADCW;
    return result;
}
#endif

#ifdef __arm__

#define GET_FUSE_VAL(base, x) ((*(base) & FUSES_##x##_Msk) >> FUSES_##x##_Pos)

ADCReadARM::ADCReadARM() : ADCReadBase()
{
    uint32_t *word1 = (uint32_t *)(NVMCTRL_TEMP_LOG);
    uint32_t *word2 = (uint32_t *)(NVMCTRL_TEMP_LOG + 4);

    tempR = GET_FUSE_VAL(word1, ROOM_TEMP_VAL_INT) * 10 +
            GET_FUSE_VAL(word1, ROOM_TEMP_VAL_DEC);
    tempH = GET_FUSE_VAL(word1, HOT_TEMP_VAL_INT) * 10 +
            GET_FUSE_VAL(word1, HOT_TEMP_VAL_DEC);
    int1VR = 1000 - (int8_t)GET_FUSE_VAL(word1, ROOM_INT1V_VAL);
    int1VH = 1000 - (int8_t)GET_FUSE_VAL(word2, HOT_INT1V_VAL);
    vadcR = GET_FUSE_VAL(word2, ROOM_ADC_VAL);
    vadcH = GET_FUSE_VAL(word2, HOT_ADC_VAL);
}

uint16_t ADCReadARM::readVcc(void)
{
    uint32_t valueRead = 0;

    // We will read VCC / 4 (0.825V for 3.3V) using the internal 1.0V reference
    analogReference(AR_INTERNAL1V0);
    delay(20);
    syncADC();
    ADC->INPUTCTRL.bit.GAIN = 0x00;      // 1X gain
    // ADC->INPUTCTRL.bin.MUXNEG = 0x18;    // Internal GND
    ADC->INPUTCTRL.bit.MUXNEG = 0x19;    // Internal I/O GND
    // ADC->INPUTCTRL.bit.MUXPOS = 0x1A;    // Internal Core VCC / 4
    ADC->INPUTCTRL.bit.MUXPOS = 0x1B;    // Internal I/O VCC / 4
    syncADC();

    valueRead = readAdc();

    // Assuming 12-bit readings
    // result = (Vcc [V] / 4) * 4096 / Vref [V]
    // we want Vcc [mV]
    // Vcc [V] = Vref [V] * result / 1024
    // Vcc [mV] = Vref [mV] * result / 1024
    valueRead *= readCorrectedVRef();
    return (uint16_t)(valueRead >> 10); // Vcc in millivolts
}

uint16_t ADCReadARM::readCorrectedVRef(int16_t *rawTemp)
{
    // We will read VCC / 4 (0.825V for 3.3V) using the internal 1.0V reference
    analogReference(AR_INTERNAL1V0);
    delay(20);
    syncADC();
    ADC->INPUTCTRL.bit.GAIN = 0x00;      // 1X gain
    ADC->INPUTCTRL.bit.MUXNEG = 0x18;    // Internal GND
    ADC->INPUTCTRL.bit.MUXPOS = 0x18;    // Temperature reference
    syncADC();

    ADC->AVGCTRL.reg = ADC_AVGCTRL_ADJRES(2) | ADC_AVGCTRL_SAMPLENUM_4;
    syncADC();

    // Read the temperature sensor (Vref is temperature dependent)
    int16_t vadcM = readAdc();
    int32_t coarse = 100 * tempR + 
                     100 * (tempH - tempR) * (vadcM - vadcR) / (vadcH - vadcR);
    int16_t int1VM = int1VR + (int1VH - int1VR) * (coarse - tempR) /
                              (tempH - tempR) / 100;

    if (rawTemp) {
        *rawTemp = vadcM;
    }

    return int1VM;
}

int16_t ADCReadARM::readCoreTemperature(void)
{
    int16_t rawTemp;
    int16_t vref = readCorrectedVRef(&rawTemp);

    uint16_t vadcM = rawTemp * vref / 1000;
    int16_t fine = 100 * tempR + 
                   100 * (tempH - tempR) * (vadcM - vadcR) / (vadcH - vadcR);
    return fine / 100;
}

inline int16_t ADCReadARM::readAdc(void)
{
    uint32_t valueRead = 0;

    // Enable the ADC, but toss the first conversion
    ADC->CTRLA.bit.ENABLE = 0x01;       // Enable ADC
    syncADC();

    // Start Conversion
    ADC->SWTRIG.bit.START = 1;
    syncADC();

    // Clear Data Ready Flag
    ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;

    // Start the conversion again
    syncADC();
    ADC->SWTRIG.bit.START = 1;

    // Waiting for conversion to complete
    while (ADC->INTFLAG.bit.RESRDY == 0);

    // Grab the value
    valueRead = ADC->RESULT.reg;

    // Disable ADC
    syncADC();
    ADC->CTRLA.bit.ENABLE = 0x00;
    syncADC();
    
    return (int16_t)valueRead;
}

int32_t ADCReadARM::mapPin(uint8_t pin, int32_t low, int32_t high)
{
    analogReference(AR_DEFAULT);

    int32_t value = map(analogRead(pin), 0, 4095, low, high);
    return value;
}

inline void ADCReadARM::syncADC(void)
{
    while (ADC->STATUS.bit.SYNCBUSY == 1);
}

#endif


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
    uint32_t value = (uint32_t)abs(m_rawVoltage);
    value *= m_device.getFullScale(m_voltageInput);
    value >>= 15;
    return (uint32_t)(float(value) / m_voltageGain);
}

uint32_t ADS1115PowerMonitor::convertCurrent(void)
{
    // reading is from 16-bit signed ADC
    // gain value is the external gain
    //  i.e. 10A -> 100mV is a gain of 0.01
    uint32_t value = (uint32_t)abs(m_rawCurrent);
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
