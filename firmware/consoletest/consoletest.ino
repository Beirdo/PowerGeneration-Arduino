#include <avr/sleep.h>

#include "sleeptimer.h"
#include "serialcli.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9340.h>

// in ms
#define LOOP_CADENCE 100

#define LCD_CS 10
#define LCD_DC 5
#define LCD_RST 7

#define LED_PWM_PIN 6

const uint8_t EEMEM rf_link_id = 0;

SleepTimer sleepTimer(LOOP_CADENCE);
Adafruit_ILI9340 tft = Adafruit_ILI9340(LCD_CS, LCD_DC, LCD_RST);

class SetLEDCLICommand : public CLICommand
{
    public:
        SetLEDCLICommand(void) : CLICommand("set_led", 1) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t value = (uint8_t)atoi(args[0]);
                Serial.print("LED value = ");
                Serial.println(value, DEC);
                analogWrite(LED_PWM_PIN, value);
                return 1;
            };
};

void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;
    
    Serial.begin(115200);

    cli.registerCommand(new SetLEDCLICommand());
    cli.initialize();

    analogWrite(LED_PWM_PIN, 5);
    tft.begin();
    //tft.fillScreen(ILI9340_BLACK);
    //tft.fillScreen(ILI9340_RED);
    //tft.fillScreen(ILI9340_GREEN);
    tft.fillScreen(ILI9340_BLUE);
    //tft.fillScreen(ILI9340_BLACK);

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
