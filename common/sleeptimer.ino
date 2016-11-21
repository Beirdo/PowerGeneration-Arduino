#include <Arduino.h>
#include "sleeptimer.h"

#define LOOP_PRESCALER 8
#define LOOP_TICK_CLOCK (CLOCK_FREQUENCY / (2 * LOOP_PRESCALER))
#define LOOP_TICK_TOP (LOOP_CADENCE * LOOP_TICK_CLOCK / 1000)
#define HI_BYTE(x)  (((x) >> 8) & 0xFF)
#define LO_BYTE(x)  ((x) & 0xFF)
#define LOOP_CS_BITS  (LOOP_PRESCALER == 1 ? (_BV(CS10)) : (LOOP_PRESCALER == 8 ? (_BV(CS11)) : (LOOP_PRESCALER == 64 ? (_BV(CS11) | _BV(CS10)) : \
                       (LOOP_PRESCALER == 256 ? (_BV(CS12)) : (LOOP_PRESCALER == 1024 ? (_BV(CS12) | _BV(CS10)) : 0)))))

uint16_t timer_count;

void TimerInitialize(void)
{
  timer_count = 0;

  // Use Timer1 as a delay timer to pace the main loop.  We want to run every x ms and sleep between
  // Prescale of 1 gives a maximum timer of 16.384ms
  // Prescale of 8 gives a maximum timer of 131.072ms
  // Prescale of 64 gives a maximum timer of 1.049s
  // Prescale of 256 gives a maximum timer of 4.194s
  // Prescale of 1024 gives a maximum timer of 16.777s
  TCCR1A = 0x00;
  TCCR1B = _BV(WGM12);  // Set it up in CTC mode, but leave it disabled
  TCCR1C = 0x00;
  OCR1AH = HI_BYTE(LOOP_TICK_TOP);
  OCR1AL = LO_BYTE(LOOP_TICK_TOP);
  TIMSK1 = _BV(OCIE1A);
}

void TimerEnable(void)
{
  // disable it
  TCCR1B = _BV(WGM12);

  // clear it
  TCNT1H = 0x00;
  TCNT1L = 0x00;

  // clear pending flag
  TIFR1 = _BV(OCF1A);

  // enable it
  TCCR1B = _BV(WGM12) | LOOP_CS_BITS;
}

void TimerDisable(void)
{
  TCCR1B = _BV(WGM12);
}

ISR(TIMER1_COMPA_vect)
{
  TimerDisable();
  timer_count++;
}

