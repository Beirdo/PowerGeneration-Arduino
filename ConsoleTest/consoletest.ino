#include <avr/sleep.h>

#include "sleeptimer.h"
#include "serialcli.h"

#define CLOCK_FREQUENCY 8000000

// in ms
#define LOOP_CADENCE 100


void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;
    
    Serial.begin(115200);

//    cli.registerCommand(BatteryCLICommand());
//    cli.registerCommand(DesulfateCLICommand());
//    cli.registerCommand(CapacityCLICommand());

    TimerInitialize();
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    noInterrupts();
    TimerEnable();

    cli.handleInput();

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
