#include <Arduino.h>
#include "sleeptimer.h"

#define LOOP_CS_BITS(x)  ((x) == 1 ? (_BV(CS10)) : ((x) == 8 ? (_BV(CS11)) : ((x) == 64 ? (_BV(CS11) | _BV(CS10)) : \
                       ((x) == 256 ? (_BV(CS12)) : ((x) == 1024 ? (_BV(CS12) | _BV(CS10)) : 0)))))

SleepTimer *globalSleepTimer = NULL;

SleepTimer::SleepTimer(uint32_t clock_freq, uint16_t loop_ms)
{
    globalSleepTimer = this;
    setCount(0);

    // Use Timer1 as a delay timer to pace the main loop. 
    // We want to run every x ms and sleep between
    TCCR1A = 0x00;
    TCCR1B = _BV(WGM12);  // Set it up in CTC mode, but leave it disabled
    TCCR1C = 0x00;

    m_cadence = loop_ms;

    if (m_cadence <= 16) {
        // Prescale of 1 gives a maximum timer of 16.384ms
        m_prescaler = 1;
    } else if (m_cadence <= 131) {
        // Prescale of 8 gives a maximum timer of 131.072ms
        m_prescaler = 8;
    } else if (m_cadence <= 1049) {
        // Prescale of 64 gives a maximum timer of 1.049s
        m_prescaler = 64;
    } else if (m_cadence <= 4194) {
        // Prescale of 256 gives a maximum timer of 4.194s
        m_prescaler = 256;
    } else {
        // Prescale of 1024 gives a maximum timer of 16.777s
        if (m_cadence > 16777) {
            m_cadence = 16777;
        }
        m_prescaler = 1024;
    }

    uint32_t loop_tick_clock = (clock_freq / (2 * m_prescaler));
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
    TCCR1B = _BV(WGM12) | LOOP_CS_BITS;
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
