#ifndef PWM_H__
#define PWM_H__

#include <Arduino.h>

class ConverterPWM {
    public:
        ConverterPWM(uint8_t *reg);
        void updateValue(uint8_t value);
        void increment(uint8_t interval);
        void decrement(uint8_t interval);
        uint8_t getValue(void) { return m_value; };
    protected:
        bool initializeTimer0(void);
        uint8_t *m_reg;
        uint8_t m_value;
};

#endif

// vim:ts=4:sw=4:ai:et:si:sts=4
