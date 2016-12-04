#ifndef SLEEPTIMER_H__
#define SLEEPTIMER_H__

class SleepTimer {
    public:
        SleepTimer(uint16_t loop_ms);
        void enable(void);
        void disable(void);
        uint16_t count(void) { return m_count; };
        void setCount(uint16_t value) { m_count = value; };
        void incrementCount(void) { m_count++; };
    protected:
        uint16_t m_count;
        uint16_t m_cadence;
        uint16_t m_prescaler;
        uint8_t m_cs_bits;
        uint16_t m_tick_top;
        uint32_t m_tick_clock;
};

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
