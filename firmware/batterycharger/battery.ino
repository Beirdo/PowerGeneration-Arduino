#include <PCF8574.h>
#include "battery.h"

#define DESULFATOR_PWM_PIN 6
#define LI_ION_IRQ_PIN 2
#define GPIO_IRQ_PIN 8

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
    {0, 3, NULL},
    {0, 7, NULL},
    {1, 0, NULL},
    {1, 1, NULL},
    {1, 2, NULL},
    {1, 4, NULL},
    {1, 5, NULL},
    {1, 6, NULL},
    {2, 0, NULL},
};


void updateIO0(void);
void updateIO1(void);
void updateIO2(void);
void updateIO(uint8_t index);

void isrWrap0(void);
void isrWrap1(void);
void isrWrap2(void);

void isrWrap0(void)
{
    pcf8574[0].checkForInterrupt();
}

void isrWrap1(void)
{
    pcf8574[1].checkForInterrupt();
}

void isrWrap2(void)
{
    pcf8574[2].checkForInterrupt();
}

void BatteryChargerInitialize(void)
{
    pcf8574[0].begin(0x20);
    pcf8574[1].begin(0x21);
    pcf8574[2].begin(0x22);

    // This is on an IO
    pcf8574[0].enableInterrupt(GPIO_IRQ_PIN, isrWrap0);
    pcf8574[0].attachInterrupt(0, updateIO0, CHANGE);
    pcf8574[0].attachInterrupt(1, updateIO0, CHANGE);
    pcf8574[0].attachInterrupt(2, updateIO0, CHANGE);
    pcf8574[0].attachInterrupt(4, updateIO0, CHANGE);
    pcf8574[0].attachInterrupt(5, updateIO0, CHANGE);
    pcf8574[0].attachInterrupt(6, updateIO0, CHANGE);

    // This is on EXTINT1
    attachInterrupt(digitalPinToInterrupt(LI_ION_IRQ_PIN), isrWrap2, FALLING);
    pcf8574[2].attachInterrupt(1, updateIO2, CHANGE);
    pcf8574[2].attachInterrupt(2, updateIO2, CHANGE);
    pcf8574[2].attachInterrupt(3, updateIO2, CHANGE);
    pcf8574[2].attachInterrupt(4, updateIO2, CHANGE);

    uint8_t count = sizeof(inputs);
    io_t *io;

    for (int i = 0; i < count; i++) {
        io = &inputs[i];
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
        io = &outputs[i];
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
                        io->variable = battery[0].desulfatePtr();
                        break;
                    case 1:
                        io->variable = battery[0].capacity9AhPtr();
                        break;
                    case 2:
                        io->variable = battery[0].capacity20AhPtr();
                        break;
                    case 4:
                        io->variable = battery[0].desulfatePtr();
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
                    default:
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
    for (int i = 0; i < 3; i++) {
        updateIO(i);
    }
}

void updateIO0(void)
{
    updateIO(0);
}

void updateIO1(void)
{
    updateIO(1);
}

void updateIO2(void)
{
    updateIO(2);
}

void updateIO(uint8_t index)
{
    if (index > 3) {
        return;
    }
    
    uint8_t values = 0;
    io_t *io;

    uint8_t count = sizeof(outputs);
    for (int i = 0; i < count; i++) {
        io = &outputs[i];
        if (io->dev == index && *io->variable) {
            values |= 1 << io->pin;
        }
    }

    pcf8574[index].write(values);
    values = pcf8574[index].read();

    count = sizeof(inputs);
    for (int i = 0; i < count; i++) {
        io = &inputs[i];
        if (io->dev == index && values & (1 << io->pin)) {
            *io->variable = 1;
        } else {
            *io->variable = 0;
        }
    }
}

Battery::Battery(uint8_t num)
{
    m_powergood = 0;

    m_num = num;

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
    m_enabled[0] = 0;
    m_enabled[1] = 0;

    setPWM(0);
    set(0, 0);
    set(1, 0);
}

void Desulfator::set(uint8_t battery, uint8_t enable)
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

void Desulfator::setPWM(uint8_t enable)
{
    if (enable) {
        analogWrite(DESULFATOR_PWM_PIN, 128);
    } else {
        analogWrite(DESULFATOR_PWM_PIN, 0);
    }
}

// vim:ts=4:sw=4:ai:et:si:sts=4
