/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <SSD1306.h>
#include <Adafruit_FRAM_SPI.h>
#include <LowPower.h>
#include "lcdscreen.h"
#include "adcread.h"

#define FRAM_CS_PIN 10

Adafruit_FRAM_SPI fram(FRAM_CS_PIN);
SSD1306 display(0x3C);
LCDDeck lcdDeck(&display, true);

#define LOOP_CADENCE 120
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

uint32_t a7;
uint16_t lcdTicks;
uint8_t lcdIndex;

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()   {                
  Serial.begin(115200);

  fram.begin();
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done

  display.attachRAM(&fram, 0x0000, 0x0400);

  //display.initializeLogo();

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();

  lcdDeck.addFrame(new LCDScreen("Core Temp", (void *)&core_temperature, formatTemperature, "C"));
  lcdDeck.addFrame(new LCDScreen("AD7", (void *)&a7, formatAutoScale, "V"));
  lcdTicks = 0;
}


void loop() {
  core_temperature = readAvrTemperature();
  a7 = ((uint32_t)(analogRead(7)) * 5000 / 1024);
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

  LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
}

