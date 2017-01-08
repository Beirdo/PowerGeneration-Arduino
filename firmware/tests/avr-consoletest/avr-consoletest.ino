#include "serialcli.h"

#include <SPI.h>
#include <LowPower.h>

// in ms
#define LOOP_CADENCE 120
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

SerialCLI CLI(Serial);

void setup() 
{
    CLI.initialize();
}

void loop() 
{
    CLI.handleInput();
    LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_ON,
                  SPI_OFF, USART0_ON, TWI_OFF);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
