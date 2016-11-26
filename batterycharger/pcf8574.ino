#include <Wire1.h>
#include "pcf8574.h"

#define GPIO_IRQ_NUM    INT1_vect_num

PCF8574 pcf8574(0x20, GPIO_IRQ_NUM);

PCF8574::PCF8574(uint8_t address, uint8_t irq)
{
    Wire1.begin();
    m_address = address;
    m_irq = irq;
    m_input_mask = 0x77;
    m_output_values = 0x77;

    sendValues(m_input_mask | (m_output_values & ~m_input_mask));
    m_read_values = readValues() & m_input_mask;
}

PCF8574::update(uint8_t mask, uint8_t value)
{
    // Active high push-pull outputs
    uint8_t outValue = m_output_values & ~mask;
    outValue |= value & mask;
    outValue &= ~m_input_mask;
    outValue |= m_input_mask;
    sendValues(outValue);
    m_output_values = outValue;
}

void PCF8574::sendValues(uint8_t value)
{
    Wire1.beginTransmission(m_address);
    Wire1.write(value);
    Wire1.endTransmission();
}

uint8_t PCF8574::readValues(void)
{
    sendValues(m_output_values);
    Wire1.requestFrom(m_address, 1);
    return Wire1.read();
}

void PCF8574::setReadValues(void)
{
    m_read_values = readValues() & m_input_mask;
}

_VECTOR(GPIO_IRQ_NUM)
{
    pcf8547.setReadValues();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
