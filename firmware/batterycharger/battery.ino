#include <PCF8574.h>
#include "battery.h"

#define DESULFATOR_PWM_PIN 6

Desulfator desulfator;
Battery battery[2] = { Battery(0), Battery(1) };
PCF8574 pcf8574[3];

typedef struct {
    uint8_t dev;
    uint8_t pin;
    uint8_t *variable;
} io_t;

uint8_t reg5VPgood;
uint8_t liIonPgood;
uint8_t liIonStat1;
uint8_t liIonStat2;

io_t inputs[] {
    {0, 0, NULL},
    {0, 1, NULL},
    {0, 2, NULL},
    {0, 4, NULL},
    {0, 5, NULL},
    {0, 6, NULL},
    {2, 1, &reg5VPgood},
    {2, 2, &liIonPgood},
    {2, 3, &liIonStat1},
    {2, 4, &liIonStat2},
};

uint8_t liIonEn;

io_t outputs[] {
    {0, 3, &enable1},
    {0, 7, &enable2},
    {1, 0, &desulfate1},
    {1, 1, &batt9ah1},
    {1, 2, &batt20ah1},
    {1, 4, &desulfate2},
    {1, 5, &batt9ah2},
    {1, 6, &batt20ah2},
    {2, 0, &liIonEn},
};


void BatteryChargerInitialize(void)
{
    pcf8574[0].begin(0x20);
    pcf8574[1].begin(0x21);
    pcf8574[2].begin(0x22);

    uint8_t count = sizeof(inputs);
    io_t *io;

    for (int i = 0; i < count; i++) {
        io = &input[i];
        pcf8574[io->dev].pinMode(io->pin, INPUT_PULLUP, false);
        switch (io->dev) {
            case 0:
                switch(io->pin) {
                    case 0:
                        io->variable = battery[0].stat1Ptr();
                        break;
                    case 1:
                        io->variable = battery[0].stat2Ptr();
                        break;
                    case 2:
                        io->variable = battery[0].pwrgoodPtr();
                        break;
                    case 4:
                        io->variable = battery[1].stat1Ptr();
                        break;
                    case 5:
                        io->variable = battery[1].stat2Ptr();
                        break;
                    case 6:
                        io->variable = battery[1].pwrgoodPtr();
                        break;
                    default:
                        break;
                }
                break;
            case 2:
                switch(io->pin) {
                    case 1:
                        // {2, 1, &reg5VPgood},
                        break;
                    case 2:
                        // {2, 2, &liIonPgood},
                        break;
                    case 3:
                        // {2, 3, &liIonStat1},
                        break;
                    case 4:
                        // {2, 4, &liIonStat2},
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    count = sizeof(outputs);
    for (int i = 0; i < count; i++) {
        io = &output[i];
        *io->variable = 0;
        pcf8574[io->dev].pinMode(io->pin, OUTPUT, false);

        switch (io->dev) {
            case 0:
                switch (io->pin) {
                    case 3:
                        io->variable = battery[0].enabledPtr();
                        break;
                    case 7:
                        io->variable = battery[1].enabledPtr();
                        break;
                    default:
                        break;
                }
                break;
            case 1:
                switch (io->pin) {
                    case 0:
                        io->variable = battery[0].desulatePtr();
                        break;
                    case 1:
                        io->variable = battery[0].capacity9AhPtr();
                        break;
                    case 2:
                        io->variable = battery[0].capacity20AhPtr();
                        break;
                    case 4:
                        io->variable = battery[0].desulatePtr();
                        break;
                    case 5:
                        io->variable = battery[0].capacity9AhPtr();
                        break;
                    case 6:
                        io->variable = battery[0].capacity20AhPtr();
                        break;
                    default:
                        break;
                }
                break;
            case 2:
                switch (io->pin) {
                    case 0:
                        // {2, 0, &liIonEn},
                        break;
                    default;
                        break;
                }
                break;
            default:
                break;
        }
    }

    updateAllIO();
}

void updateAllIO(void)
{
    uint8_t values[3] = {0, 0, 0};

    uint8_t count = sizeof(outputs);
    for (int i = 0; i < count; i++) {
        io = &output[i];
        if (*io->variable) {
            value[i] |= 1 << io->pin;
        }
    }

    for (int i = 0; i < 3; i++) {
        pcf8574[i].write(value[i]);
        values[i] = pcf8574[i].read();
    }

    count = sizeof(inputs);
    for (int i = 0; i < count; i++) {
        io = &input[i];
        if (value[i] & (1 << io->pin)) {
            *io->variable = 1;
        } else {
            *io->variable = 0;
        }
    }
}

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

    m_capacity9Ah  = (capacity == 9);
    m_capacity20Ah = (capacity == 20);
}

void Battery::setDesulfate(uint8_t desulfate)
{
    m_desulfate = desulfate;
}

void Battery::setEnabled(uint8_t enabled)
{
    m_enabled = enabled;
}

Desulfator::Desulfator(void)
{
    m_enabled = 0;

    setPWM(0);
    set(0, 0);
    set(1, 0);
}

Desulfator::set(uint8_t battery, uint8_t enable)
{
    bool oldEnabled = (m_enabled[0] || m_enabled[1]);
    m_enabled[battery] = enable;
    bool enabled = (m_enabled[0] || m_enabled[1]);

    if (!oldEnabled && enabled) {
        setPWM(1);
    }

    if (oldEnabled && !enabled) {
        setPWM(0);
    }
}

Desulfator::setPWM(uint8_t enable)
{
    if (enable) {
        analogWrite(DESULFATOR_PWM_PIN, 128);
    } else
        analogWrite(DESULFATOR_PWM_PIN, 0);
    }
}

// vim:ts=4:sw=4:ai:et:si:sts=4
