#include <SHA204.h>
#include <SHA204Definitions.h>
#include <Wire.h>
#include <SHA204I2C.h>

SHA204I2C sha204dev;

void Sha204Initialize(void)
{
    Serial.println("Starting I2C");
    Wire.begin();

    // Be sure to wake up device right as I2C goes up otherwise you'll have
    // NACK issues
    sha204dev.init();
    Sha204Wakeup();
}

byte Sha204Wakeup(void)
{
    uint8_t response[SHA204_RSP_SIZE_MIN];
    byte returnValue;
    
    returnValue = sha204dev.resync(4, &response[0]);
    for (int i = 0; i < SHA204_RSP_SIZE_MIN; i++) {
        Serial.print(response[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    return returnValue;
}

byte Sha204GetSerialNumber(void)
{
    uint8_t serialNumber[9];
    byte returnValue;
    
    returnValue = sha204dev.serialNumber(serialNumber);
    for (int i = 0; i < 9; i++) {
        Serial.print(serialNumber[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
 
    Serial.println("-------"); 
    
    return returnValue;
}

byte Sha204MacChallenge(void)
{
    uint8_t command[MAC_COUNT_LONG];
    uint8_t response[MAC_RSP_SIZE];

    const uint8_t challenge[MAC_CHALLENGE_SIZE] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    uint8_t ret_code = sha204dev.execute(SHA204_MAC, 0, 0, MAC_CHALLENGE_SIZE, 
        (uint8_t *) challenge, 0, NULL, 0, NULL, sizeof(command), &command[0], 
        sizeof(response), &response[0]);

    for (int i = 0; i < SHA204_RSP_SIZE_MAX; i++) {
        Serial.print(response[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
    
    return ret_code;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
