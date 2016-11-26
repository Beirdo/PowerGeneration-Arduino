#ifndef PCF8574_H__
#define PCF8574_H__

class PCF8574 {
    public:
        PCF8574(uint8_t address, uint8_t irq);
        void update(uint8_t mask, uint8_t value);
        uint8_t read(void) { return m_read_values; };
        void setReadValues(void);

    protected:
        void sendValues(uint8_t value);
        uint8_t readValues(void);
        void ISR(void);

        uint8_t m_address;
        uint8_t m_input_mask;
        uint8_t m_output_values;
        uint8_t m_read_values;
}

extern PCF8574 pcf8574;

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
