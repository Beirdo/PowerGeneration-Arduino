#include <LowPower.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "adcread.h"
#include "lcdscreen.h"

// in ms
#define LOOP_CADENCE 120
#define BLANK_TIME 2000
#define BLANK_COUNT (BLANK_TIME / LOOP_CADENCE)

uint32_t blankTicks;
uint32_t current;
uint32_t voltage;
uint8_t enabled;
INA219PowerMonitor monitor(0x40, 12, 160, 110, 1.455);
LiquidCrystal_I2C LCD(0x20, 16, 2);

void setup(void)
{
    LCD.init();
    LCD.noDisplay();

    current = 0;
    voltage = 0;
    enabled = 0;
    blankTicks = 0;
}

void loop(void)
{
    uint8_t buffer[10];
    uint8_t prevEnabled;

    if (monitor.readMonitor()) {
        voltage = monitor.voltage();
        current = monitor.current();
    }

    prevEnabled = enabled;
    if (voltage > 50) {
        blankTicks = 0;
        enabled = 1;
    } else {
        blankTicks++;
        if (blankTicks >= BLANK_COUNT) {
            if (enabled) {
                enabled = 0;
            }
            blankTicks -= BLANK_COUNT;
        }
    }

    if (enabled && !prevEnabled) {
        // initialize LCD
        LCD.backlight();
        LCD.display();
        LCD.noCursor();
    } else if (!enabled && prevEnabled) {
        // shut off LCD
        LCD.clear();
        LCD.noDisplay();
        LCD.noBacklight();
    }

    if (enabled) {
        // Update LCD
        LCD.clear();
        LCD.setCursor(0, 0);
        LCD.print("Voltage: ");
        formatAutoScaleMilli(&voltage, buffer, 7, "V");
        LCD.print((char *)buffer);
        LCD.setCursor(1, 0);
        LCD.print("Current: ");
        formatAutoScaleMilli(&current, buffer, 7, "A");
        LCD.print((char *)buffer);
    }

    LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_ON,
                  SPI_OFF, USART0_OFF, TWI_OFF);
}


// vim:ts=4:sw=4:ai:et:si:sts=4
