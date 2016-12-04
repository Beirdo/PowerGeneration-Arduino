#include <Arduino.h>
#include "sleeptimer.h"

// tickclk(x) = F_CPU / (2 * x)
// maxtime(x) = 65535 / tickclk(x)
// maxms(x) = 1000 * 65535 * 2 * x / F_CPU
#define MAX_TIME(x) (1000 * 0xFFFF * 2 * (x) / F_CPU)

SleepTimer *globalSleepTimer = NULL;

SleepTimer::SleepTimer(uint16_t loop_ms)
{
    globalSleepTimer = this;
    setCount(0);

    // Use Timer1 as a delay timer to pace the main loop. 
    // We want to run every x ms and sleep between
    TCCR1A = 0x00;
    TCCR1B = _BV(WGM12);  // Set it up in CTC mode, but leave it disabled
    TCCR1C = 0x00;

    m_cadence = loop_ms;

    if (m_cadence <= MAX_TIME(1)) {
        // Prescale of 1 gives a maximum timer of 16.384ms
        m_prescaler = 1;
        m_cs_bits = _BV(CS10);
    } else if (m_cadence <= MAX_TIME(8)) {
        // Prescale of 8 gives a maximum timer of 131.072ms
        m_prescaler = 8;
        m_cs_bits = _BV(CS11);
    } else if (m_cadence <= MAX_TIME(64)) {
        // Prescale of 64 gives a maximum timer of 1.049s
        m_prescaler = 64;
        m_cs_bits = _BV(CS11) | _BV(CS10);
    } else if (m_cadence <= MAX_TIME(256)) {
        // Prescale of 256 gives a maximum timer of 4.194s
        m_prescaler = 256;
        m_cs_bits = _BV(CS12);
    } else {
        // Prescale of 1024 gives a maximum timer of 16.777s
        if (m_cadence > MAX_TIME(1024)) {
            m_cadence = MAX_TIME(1024);
        }
        m_prescaler = 1024;
        m_cs_bits = _BV(CS12) | _BV(CS10);
    }

    uint32_t loop_tick_clock = (F_CPU / (2 * m_prescaler));
    uint16_t loop_tick_top = (uint16_t)((uint32_t)m_cadence * loop_tick_clock / 1000L);

    OCR1AH = highByte(loop_tick_top);
    OCR1AL = lowByte(loop_tick_top);
    TIMSK1 = _BV(OCIE1A);
}

void SleepTimer::enable(void)
{
    // disable it
    TCCR1B = _BV(WGM12);

    // clear it
    TCNT1H = 0x00;
    TCNT1L = 0x00;

    // clear pending flag
    TIFR1 = _BV(OCF1A);

    // enable it
    TCCR1B = _BV(WGM12) | m_cs_bits;
}

void SleepTimer::disable(void)
{
    TCCR1B = _BV(WGM12);
}

ISR(TIMER1_COMPA_vect)
{
    if (globalSleepTimer) {
        globalSleepTimer->disable();
        globalSleepTimer->incrementCount();
    }
}

// vim:ts=4:sw=4:ai:et:si:sts=4
