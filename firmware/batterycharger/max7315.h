#ifndef MAX7315_H__
#define MAX7315_H__

#define MAX7315_REG_READ_INPUTS         0x00
#define MAX7315_REG_BLINK_PH_0_OUTPUTS  0x01
#define MAX7315_REG_PORT_CONFIG         0x03
#define MAX7315_REG_BLINK_PH_1_OUTPUTS  0x09
#define MAX7315_REG_MASTER_OUTPUT8      0x0E
#define MAX7315_REG_CONFIG              0x0F
#define MAX7315_REG_OUTPUT_INTENSITY_01 0x10
#define MAX7315_REG_OUTPUT_INTENSITY_23 0x11
#define MAX7315_REG_OUTPUT_INTENSITY_45 0x12
#define MAX7315_REG_OUTPUT_INTENSITY_57 0x13

class MAX7315 {
    public:
        MAX7315(uint8_t address);
        void update(uint8_t mask, uint8_t value);
        void setOutput8PWM(uint8_t enable);

    protected:
        void sendRegister(uint8_t regNum, uint8_t value);
        void sendRegisters(uint8_t regNum, uint32_t value32);
        uint8_t m_address;
        uint8_t m_port_values;
}

extern MAX7315 max7315;

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
