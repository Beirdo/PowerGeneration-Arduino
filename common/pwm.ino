
uint8_t *timer_port_conv1 = 0;
uint8_t *timer_port_conv2 = 0;
uint8_t *timer_port_led = 0;

uint8_t pwm_conv1 = 0x00;
uint8_t pwm_conv2 = 0x00;
uint8_t pwm_led = 0x00;

void PWMInitialize(uint8_t *conv1, uint8_t *conv2, uint8_t *led)
{
  uint8_t tccr = 0;
  // Timer 0
  if (conv1 == OCR0A || conv1 == OCR0B || conv2 == OCR0A || conv2 == OCR0B)  {
    // Converter PWMs on Timer 0
    timer_port_conv1 = conv1;
    timer_port_conv2 = conv2;

    // Set the output pins
    if (conv1 == OCR0A || conv2 == OCR0A) {
      pinMode(6, OUTPUT);
      tccr |= _BV(COM0A1);
    }

    if (conv1 == OCR0B || conv2 == OCR0B) {
      pinMode(5, OUTPUT);
      tccr |= _BV(COM0B1);
    }

    // Set both OC0A and OC0B to Fast PWM mode
    // starting from BOTTOM, going to TOP, clearing output at OCRA
    TCCR0A = tccr | _BV(WGM01) | _BV(WGM00);
    // prescale of 1 for a PWM frequency of 31.25kHz (from 8MHz)
    TCCR0B = _BV(CS00);
  } else if (led == OCR0A || led == OCR0B) {
    // LED PWM on Timer 0
    timer_port_led = led;

    // Set the output pins
    if (led == OCR0A) {
      pinMode(6, OUTPUT);
      tccr |= _BV(COM0A1);
    }

    if (led == OCR0B) {
      pinMode(5, OUTPUT);
      tccr |= _BV(COM0B1);
    }

    // Set both OC0A and OC0B to Fast PWM mode
    // starting from BOTTOM, going to TOP, clearing output at OCRA
    TCCR0A = tccr | _BV(WGM01) | _BV(WGM00);
    // Prescaler of 64 for a PWM frequency of 488Hz (from 8MHz)
    TCCR0B = _BV(CS02);
  } else if (led == OCR2A || led == OCR2B) {
    // LED PWM on Timer 2
    timer_port_led = led;

    // Set the output pins
    if (led == OCR2A) {
      pinMode(11, OUTPUT);
      tccr |= _BV(COM2A1);
    }

    if (led == OCR2B) {
      pinMode(3, OUTPUT);
      tccr |= _BV(COM2B1);
    }

    // Setup OC2B to Fast PWM mode.
    TCCR2A = tccr | _BV(WGM21) | _BV(WGM20);
    // Prescaler of 64 for a PWM frequency of 488Hz (from 8MHz)
    TCCR2B = _BV(CS22);
  }
}

void PWMUpdateLed(uint16_t light_value)
{
  uint8_t value = 0xFF - (uint8_t)(light_value >> 2);
  pwm_led = value;

  if (timer_port_led) {
    *timer_port_led = value;
  }
}

void PWMUpdateConverter(uint16_t conversion, uint8_t value)
{
  if (conversion == 1) {
    pwm_conv1 = value;
    if (timer_port_conv1) {
      *timer_port_conv1 = value;
    }
  } else if (conversion == 2) {
    pwm_conv2 = value;
    if (timer_port_conv2) {
      *timer_port_conv2 = value;
    }
  }
}

// vim:ts=4:sw=4:ai:et:si:sts=4
