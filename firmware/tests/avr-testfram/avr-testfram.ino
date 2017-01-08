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
#include <Adafruit_FRAM_SPI.h>
#include <LowPower.h>

#define FRAM_CS_PIN 10

Adafruit_FRAM_SPI fram(FRAM_CS_PIN);

#define LOOP_CADENCE 120

void setup()   {                
  uint8_t data[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  Serial.begin(115200);

  fram.begin();
  fram.writeEnable(true);

  //fram.write(0x800, data, strlen(data));
}


void loop() {
  uint8_t data[65];

  fram.read(0x800, data, 64);
  data[64] = '\0';
  Serial.println((char *)data);
  
  LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
}

