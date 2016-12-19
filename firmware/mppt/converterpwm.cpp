#include <Arduino.h>
#include "converterpwm.h"

bool timer0_initialized = false;

ConverterPWM::ConverterPWM(uint8_t *reg)
{
    m_reg = reg;
    m_value = 0;
    if (!timer0_initialized) {
        timer0_initialized = initializeTimer0();
    }
}

bool ConverterPWM::initializeTimer0(void)
{
    // Set both outputs to 0
    OCR0A = 0x00;
    OCR0B = 0x00;

    // Set the output pins
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);

    // Set both OC0A and OC0B to Fast PWM mode
    // starting from BOTTOM, going to TOP, clearing output at OCRA
    TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);

    // prescale of 1 for a PWM frequency of 31.25kHz (from 8MHz)
    TCCR0B = _BV(CS00);

    return true;
}

void ConverterPWM::updateValue(uint8_t value)
{
    m_value = value;
    *m_reg = value;
}

void ConverterPWM::increment(uint8_t interval)
{
    int16_t newValue = m_value + interval;
    updateValue((uint8_t)constrain(newValue, 0, 255));
}

void ConverterPWM::decrement(uint8_t interval)
{
    int16_t newValue = m_value - interval;
    updateValue((uint8_t)constrain(newValue, 0, 255));
}

// vim:ts=4:sw=4:ai:et:si:sts=4
