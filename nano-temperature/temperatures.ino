#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>

#include "temperatures.h"


OneWire oneWire;
DallasTemperature sensors(&oneWire);

void printOneWireAddress(DeviceAddress deviceAddress);

void printOneWireAddress(DeviceAddress deviceAddress)
{
    Serial.print("{ ");
    for (uint8_t i = 0; i < 8; i++)
    {
        // zero pad the address if necessary
        Serial.print("0x");
        if (deviceAddress[i] < 16) {
            Serial.print("0");
        }
        Serial.print(deviceAddress[i], HEX);
        if (i < 7) {
            Serial.print(", ");
        }
    }
    Serial.print(" }");
}


void TemperaturesInitialize(void)
{
    Serial.println("DS12B20 Search");

    sensors.begin();
    
    DeviceAddress currAddress;
    uint8_t numberOfDevices = sensors.getDeviceCount();
    
    for (int i = 0; i < numberOfDevices; i++)
    {
        sensors.getAddress(currAddress, i);
        printOneWireAddress(currAddress);
        Serial.println();  
    }

    // Limit the resolution to 9 bits for faster readings
    sensors.setResolution(9);
}

void TemperaturesPoll(uint16_t *values, uint8_t count)
{
    Serial.println("Requesting temperatures...");
    sensors.requestTemperatures();
    
    DeviceAddress currAddress;
    uint8_t numberOfDevices = sensors.getDeviceCount();

    numberOfDevices = numberOfDevices > count ? count : numberOfDevices;
    
    for (uint8_t i = 0; i < numberOfDevices; i++)
    {
        sensors.getAddress(currAddress, i);
        values[i] = sensors.getTemp(currAddress);

        Serial.print("Temperature ");
        Serial.print(i);
        Serial.print(": ");
        Serial.print(sensors.rawToCelsius(values[i]));
        Serial.println();  
    }
}

// vim:ts=4:sw=4:ai:et:si:sts=4
