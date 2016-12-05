#include <Wire1.h>
#include "max7135.h"

MAX7315 max7315(0x21);

MAX7315::MAX7315(uint8_t address)
{
    Wire1.begin();
    m_address = address;
    m_port_values = 0x77;   // All outputs off

    // Disable global intensity, set 08 as an output driven low
    sendRegister(MAX7315_REG_CONFIG, 0x00);

    // Disable the PWM output
    sendRegister(MAX7315_REG_MASTER_OUTPUT8, 0x00);

    // Set all the output intensities to full on/off
    sendRegisters(MAX7315_REG_OUTPUT_INTENSITY_01, 0xFFFFFFFF);

    // Setup outputs (remember, these are inverted)
    sendRegister(MAX7315_REG_BLINK_PH_0_OUTPUTS, m_port_values);
    sendRegister(MAX7315_REG_PORT_CONFIG, 0x77);
}

MAX7315::update(uint8_t mask, uint8_t value)
{
    // Active low OD outputs with an inverter after them
    // For a high value, drive low, for a low value, float high
    uint8_t outValue = m_port_values & ~mask;
    outValue |= ~value & mask;
    sendRegister(MAX7315_REG_BLINK_PH_0_OUTPUTS, outValue;
    m_port_values = outValue;
}

MAX7315::setOutput8PWM(uint8_t enable)
{
    // 50% PWM when on, otherwise shut PWM right off
    sendRegister(MAX7315_REG_MASTER_OUTPUT8, enable ? 0x17 : 0x00);
}

void MAX7315::sendRegister(uint8_t regNum, uint8_t value)
{
    Wire1.setClock(400000L);
    Wire1.beginTransmission(m_address); // transmit to device #8
    Wire1.write(regNum);                // sends one byte
    Wire1.write(value);                 // sends one byte
    Wire1.endTransmission();            // stop transmitting
}

void MAX7315::sendRegisters(uint8_t regNum, uint32_t value32)
{
    uint8_t value;
    Wire1.setClock(400000L);
    Wire1.beginTransmission(m_address); // transmit to device #8
    Wire1.write(regNum);                // sends one byte
    for (uint8_t i = 0; i < 4; i++) {   // sends four bytes
        value = (uint8_t)(value32 & 0xFF);
        value32 >> = 8;
        Wire1.write(value);
    }
    Wire1.endTransmission();            // stop transmitting
}

// vim:ts=4:sw=4:ai:et:si:sts=4
