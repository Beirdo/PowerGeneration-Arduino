#ifndef BATTERY_H__
#define BATTERY_H__

class Desulfator {
    public:
        Desulfator(void);
        void set(uint8_t battery, uint8_t enable);

    protected:
        void setPWM(uint8_t enable);
        uint8_t m_enabled;
        uint8_t m_desulfate_mask[2];
}

class Battery {
    public:
        Battery(uint8_t num);
        uint8_t getCapacity(void)  { return m_capacity; };
        uint8_t getDesulfate(void) { return m_desulfate; };
        uint8_t getEnabled(void)   { return m_enabled; };
        uint8_t getState(void)     { return m_state; };
        uint8_t getPowerGood(void) { return m_powergood; };
        void setCapacity(uint8_t capacity);
        void setDesulfate(uint8_t desulfate);
        void setEnabled(uint8_t enabled);
        void updateState(void);

    protected:
        uint8_t m_num;
        uint8_t m_enabled;
        uint8_t m_desulfate;
        uint8_t m_capacity;
        uint8_t m_state;
        uint8_t m_powergood;

        uint8_t m_enable_mask;
        uint8_t m_status_mask;
        uint8_t m_status_shift;
        uint8_t m_powergood_mask;
        
        uint8_t m_9ah_mask;
        uint8_t m_20ah_mask;

};

extern Battery battery[2];


#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
