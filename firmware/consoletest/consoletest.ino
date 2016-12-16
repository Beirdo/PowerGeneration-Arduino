#include <avr/sleep.h>

#include "sleeptimer.h"
#include "serialcli.h"

// in ms
#define LOOP_CADENCE 100

const uint8_t EEMEM rf_link_id = 0;

SleepTimer sleepTimer(LOOP_CADENCE);

void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;
    
    Serial.begin(115200);

    cli.initialize();
}

void loop() 
{
    noInterrupts();
    sleepTimer.enable();

    cli.handleInput();

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
