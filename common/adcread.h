#ifndef ADCREAD_H__
#define ADCREAD_H__

extern uint16_t adc_readings[8];	///< raw readings
extern int16_t core_temperature;	///< in 1/10 degree C
extern uint16_t vcc;			///< in millivolts

uint16_t readVcc(void);
int16_t readAvrTemperature(void);
uint32_t convertVoltage(uint8_t input, uint16_t factor);
uint32_t convertCurrent(uint8_t input, uint16_t factor1, uint16_t factor2);
uint32_t calculatePower(uint32_t voltage, uint32_t current);
void ADCPoll(void);

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
