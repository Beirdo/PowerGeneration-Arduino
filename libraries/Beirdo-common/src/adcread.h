#ifndef ADCREAD_H__
#define ADCREAD_H__

#include <INA219.h>
#include <ADS1115.h>

class ADCReadBase {
    public:
        ADCReadBase() {};
        virtual int16_t readCoreTemperature(void) = 0;  ///< in 1/0 degree C
        virtual uint16_t readVcc(void) = 0;             ///< in mV
        virtual int32_t readPin(uint8_t pin, int32_t low, int32_t high) = 0;
    protected:
        virtual inline int16_t readAdc(void) = 0;
};

#ifdef __AVR__
class ADCReadAVR : public ADCReadBase {
    public:
        ADCReadAVR();
        virtual int16_t readCoreTemperature(void);  ///< in 1/0 degree C
        virtual uint16_t readVcc(void);             ///< in mV
        virtual int32_t readPin(uint8_t pin, int32_t low, int32_t high);
    protected:
        virtual inline int16_t readAdc(void);
};
#define ADCRead ADCReadAVR
#endif

#ifdef __arm__
class ADCReadARM : public ADCReadBase {
    public:
        ADCReadARM();
        virtual int16_t readCoreTemperature(void);  ///< in 1/10 deg C
        virtual uint16_t readVcc(void);             ///< in mV
        virtual int32_t mapPin(uint8_t pin, int32_t low, int32_t high);
    protected:
        virtual inline int16_t readAdc(void);
        uint16_t readCorrectedVRef(int16_t *rawTemp = NULL);
        int16_t tempR;      ///< in 1/10 deg C
        int16_t tempH;      ///< in 1/10 deg C
        uint16_t int1VR;    ///< in mV
        uint16_t int1VH;    ///< in mV
        uint32_t vadcR;     ///< 12-bit ADC reading
        uint32_t vadcH;     ///< 12-bit ADC reading
};
#define ADCRead ADCReadARM
#endif

class PowerMonitor {
    public:
        PowerMonitor(void);
        bool readMonitor(void);
        uint32_t current(void) { return m_current; };
        uint32_t voltage(void) { return m_voltage; };
        uint32_t power(void)   { return m_power; };
    protected:
        virtual bool readCurrent(uint32_t &value) = 0;
        virtual bool readVoltage(uint32_t &value) = 0;
        virtual bool readPower(uint32_t &value) = 0;
        uint32_t m_current;
        uint32_t m_voltage;
        uint32_t m_power;
};

class INA219PowerMonitor : public PowerMonitor {
    public:
        INA219PowerMonitor(uint8_t address, uint8_t maxVoltage,
                           uint16_t maxVShunt, uint16_t rShunt,
                           float maxIExpected);
    protected:
        INA219 m_device;
        virtual bool readCurrent(uint32_t &value);
        virtual bool readVoltage(uint32_t &value);
        virtual bool readPower(uint32_t &value);
        uint8_t m_address;
};

class ADS1115PowerMonitor : public PowerMonitor {
    public:
        ADS1115PowerMonitor(uint8_t address, uint8_t current, uint8_t voltage,
                            uint8_t currentPga, uint8_t voltagePga,
                            float currentGain, float voltageGain);
    protected:
        ADS1115 m_device;
        virtual bool readCurrent(uint32_t &value);
        virtual bool readVoltage(uint32_t &value);
        virtual bool readPower(uint32_t &value);
        uint8_t m_address;
        uint8_t m_currentInput;
        uint8_t m_voltageInput;
        uint8_t m_currentPga;
        uint8_t m_voltagePga;
        float m_currentGain;
        float m_voltageGain;

        int16_t m_rawCurrent;
        int16_t m_rawVoltage;

        uint32_t m_lastCurrent;
        uint32_t m_lastVoltage;

        uint32_t convertCurrent(void);
        uint32_t convertVoltage(void);
        uint32_t calculatePower(void);
};

#endif

// vim:ts=4:sw=4:ai:et:si:sts=4
