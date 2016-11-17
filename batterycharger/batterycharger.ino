#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>

#include <SPI.h>

#include <SD.h>

#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>


#include <Adafruit_GFX.h>
#include <gfxfont.h>

#include <Adafruit_ILI9340.h>

OneWire oneWire;
DallasTemperature sensors(&oneWire);


void printAddress(DeviceAddress deviceAddress)
{
  Serial.print("{ ");
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    Serial.print("0x");
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i<7) Serial.print(", ");
    
  }
  Serial.print(" }");
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("DS12B20 Searchh");
    
  sensors.begin();
  
  DeviceAddress currAddress;
  uint8_t numberOfDevices = sensors.getDeviceCount();
  
  for (int i=0; i<numberOfDevices; i++)
  {
    sensors.getAddress(currAddress, i);
    printAddress(currAddress);
    Serial.println();  
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Requesting temperatures...");
  sensors.requestTemperatures();
  
  DeviceAddress currAddress;
  uint8_t numberOfDevices = sensors.getDeviceCount();
  
  for (int i=0; i<numberOfDevices; i++)
  {
    sensors.getAddress(currAddress, i);
    printAddress(currAddress);
    Serial.print(": ");
    Serial.print(sensors.getTempCByIndex(i));
    Serial.println();  
  }
  
  delay(10000);
}
