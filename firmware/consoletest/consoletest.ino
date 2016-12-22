#include "serialcli.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <LowPower.h>

// in ms
#define LOOP_CADENCE 120

#define LCD_CS 10
#define LCD_DC 5
#define LCD_RST 7

#define LED_PWM_PIN 6

const uint8_t EEMEM rf_link_id = 0;

Adafruit_ILI9341 tft = Adafruit_ILI9341(LCD_CS, LCD_DC, LCD_RST);

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
    Serial.begin(115200);

    cli.registerCommand(new SetLEDCLICommand());
    cli.initialize();

    analogWrite(LED_PWM_PIN, 5);
    tft.begin();
    //tft.fillScreen(ILI9341_BLACK);
    //tft.fillScreen(ILI9341_RED);
    //tft.fillScreen(ILI9341_GREEN);
    tft.fillScreen(ILI9341_BLUE);
    //tft.fillScreen(ILI9341_BLACK);

}

void loop() 
{
    cli.handleInput();
    delay(LOOP_CADENCE);
    //LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_ON, TWI_OFF);

}

// vim:ts=4:sw=4:ai:et:si:sts=4
