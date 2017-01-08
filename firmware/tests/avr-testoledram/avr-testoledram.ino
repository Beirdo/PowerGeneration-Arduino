#include <Adafruit_GFX.h>
#include <SSD1306.h>
#include <Adafruit_FRAM_SPI.h>
#include <LowPower.h>
#include "lcdscreen.h"
#include "adcread.h"

#define FRAM_CS_PIN 10

Adafruit_FRAM_SPI fram(FRAM_CS_PIN);
SSD1306 display(0x3C);
LCDDeck lcdDeck(&display);

#define LOOP_CADENCE 120
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

ADCRead adcread;
int16_t core_temperature;
uint32_t vcc;
uint16_t lcdTicks;
uint8_t lcdIndex;

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix SSD1306.h!");
#endif

void setup()   
{                
    Serial.begin(115200);

    fram.begin();
    
    // by default, we'll generate the high voltage from the 3.3v line
    // internally! (neat!)

    // initialize with the default I2C addr (0x3C)
    display.begin(SSD1306_SWITCHCAPVCC);

    // attach the FRAM
    display.attachRAM(&fram, 0x0000, 0x0400);

    // Initialize the logo in the FRAM
    // display.initializeLogo();

    // Show image buffer on the display hardware.
    // Since the buffer is intialized with an Adafruit splashscreen
    // internally, this will display the splashscreen.
    display.display();

    lcdDeck.addFrame(new LCDScreen("Core Temp", (void *)&core_temperature,
                     formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Vcc", (void *)&vcc, formatAutoScale, "V"));
    lcdTicks = 0;
}


void loop()
{
    core_temperature = adcread.readCoreTemperature();
    vcc = adcread.readVcc();
    uint32_t start;
    uint32_t now;

    lcdTicks++;
    if (lcdTicks >= SWAP_COUNT) {
        lcdTicks -= SWAP_COUNT;

        lcdIndex = lcdDeck.nextIndex();
 
        start = micros();
        lcdDeck.formatFrame(lcdIndex);
        now = micros();
        Serial.print("F ");
        Serial.print(now - start, DEC);
        Serial.println("us");
        
        start = now;
        lcdDeck.displayFrame();
        now = micros();
        Serial.print("D ");
        Serial.print(now - start, DEC);
        Serial.println("us");
    }

    LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                  SPI_OFF, USART0_OFF, TWI_OFF);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
