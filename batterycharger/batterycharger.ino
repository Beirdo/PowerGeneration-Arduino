
#include <SPI.h>

#include <SdFat.h>

#include "rflink.h"
#include "temperatures.h"

#include <Adafruit_GFX.h>
#include <gfxfont.h>

#include <Adafruit_ILI9340.h>

SdFat sdcard;

uint16_t temperatures[8];

#define RF_CS_PIN 4
#define RF_CE_PIN 5

void setup() 
{
  Serial.begin(9600);

  TemperaturesInitialize();
  RFLinkInitialize(2, 0xFF);
}

void loop() 
{
  TemperaturesPoll(temperatures, 8);
  
  delay(10000);
}
