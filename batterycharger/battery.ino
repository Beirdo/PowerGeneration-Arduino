#include "battery.h"

Desulfator desulfator;
Battery battery[2] = { Battery(0), Battery(1) };

Battery::Battery(uint8_t num)
{
    m_state = 0;
    m_powergood = 0;

    m_num = num;
    if (num == 0) {
        m_enable_mask = _BV(3);
        m_status_mask = _BV(1) | _BV(0);
        m_status_shift = 0;
        m_powergood_mask = _BV(2);
        m_desulfate_mask = _BV(0);
        m_9ah_mask = _BV(1);
        m_20ah_mask = _BV(2);
    } else {
        m_enable_mask = _BV(7);
        m_status_mask = _BV(5) | _BV(4);
        m_status_shift = 4;
        m_powergood_mask = _BV(6);
        m_desulfate_mask = _BV(4);
        m_9ah_mask = _BV(5);
        m_20ah_mask = _BV(6);
    }

    setEnabled(0);
    setDesulfate(0);
    setCapacity(9);
}

void Battery::setCapacity(uint8_t capacity)
{
    if (capacity != 9 && capacity != 20) {
        capacity = 9;
    }

    m_capacity = capacity;

    uint8_t mask = m_9ah_mask | m_20ah_mask;
    uint8_t value;

    if (capacity == 9) {
        value = m_9ah_mask;
    } else {
        value = m_20ah_mask;
    }

    max7315.update(mask, value);
}

void Battery::setDesulfate(uint8_t desulfate)
{
    m_desulfate = desulfate;
    desulfator.set(m_num, desulfate);
}

void Battery::setEnabled(uint8_t enabled)
{
    m_enabled = enabled;
    pcf8574.update(m_enable_mask, enabled ? m_enable_mask : 0);
}

void Battery::setState(uint8_t state)
{
    m_state = (state & m_status_mask) >> m_status_shift;
    m_powergood = !(!(state & m_powergood_mask));
}

Desulfator::Desulfator(void)
{
    m_desulfate_mask[0] = _BV(0);
    m_desulfate_mask[1] = _BV(4);
    m_enabled = 0;

    setPWM(0);
    set(0, 0);
    set(1, 0);
}

Desulfator::set(uint8_t battery, uint8_t enable)
{
    uint8_t mask = m_desulfate_mask[battery];
    uint8_t value = m_enabled;

    if (enabled) {
        value |= mask;
    } else {
        value &= ~mask;
    }

    if (value != m_enabled) {
        max7315.update(mask, value);
        if (!m_enabled) {
            setPWM(1);
        }
        m_enabled = value;
        if (!m_enabled) {
            setPWM(0);
        }
    }
}

Desulfator::setPWM(uint8_t enable)
{
    max7315.setOutput8PWM(enable);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
