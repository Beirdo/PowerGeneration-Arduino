#ifndef PWM_H__
#define PWM_H__

#include <Arduino.h>

extern uint8_t pwm_conv1;
extern uint8_t pwm_conv2;
extern uint8_t pwm_led;

void PWMInitialize(uint8_t *conv1, uint8_t *conv2, uint8_t *led);
void PWMUpdateLed(uint16_t light_value);
void PWMUpdateConverter(uint16_t conversion, uint8_t value);

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
