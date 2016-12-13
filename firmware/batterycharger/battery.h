#ifndef BATTERY_H__
#define BATTERY_H__

void BatteryChargerInitialize(uint8_t addr0, uint8_t addr1, uint8_t addr2);
void updateAllIO(void);

class Desulfator {
    public:
        Desulfator(void);
        void set(uint8_t battery, uint8_t enable);

    protected:
        void setPWM(uint8_t enable);
        uint8_t m_enabled[2];
};

class Battery {
    public:
        Battery(uint8_t num);
        uint8_t getCapacity(void)   { return m_capacity; };
        uint8_t getDesulfate(void)  { return m_desulfate; };
        uint8_t getEnabled(void)    { return m_enabled; };
        uint8_t getState(void)      { return (m_stat2 << 1) | m_stat1; };
        uint8_t getPowerGood(void)  { return m_powergood; };
        void setCapacity(uint8_t capacity);
        void setDesulfate(uint8_t desulfate);
        void setEnabled(uint8_t enabled);

        uint8_t *enabledPtr(void)       { return &m_enabled; };
        uint8_t *desulfatePtr(void)     { return &m_desulfate; };
        uint8_t *pwrgoodPtr(void)       { return &m_powergood; };
        uint8_t *capacity9AhPtr(void)   { return &m_capacity9Ah; };
        uint8_t *capacity20AhPtr(void)  { return &m_capacity20Ah; };
        uint8_t *stat1Ptr(void)         { return &m_stat1; };
        uint8_t *stat2Ptr(void)         { return &m_stat2; };

    protected:
        uint8_t m_num;
        uint8_t m_capacity;

        uint8_t m_enabled;
        uint8_t m_desulfate;
        uint8_t m_capacity9Ah;
        uint8_t m_capacity20Ah;
        uint8_t m_powergood;
        uint8_t m_stat1;
        uint8_t m_stat2;
};

extern Battery battery[2];


#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
