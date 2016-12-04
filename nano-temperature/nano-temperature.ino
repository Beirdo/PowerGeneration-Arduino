#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "rflink.h"
#include "temperatures.h"
#include "sleeptimer.h"
#include "adcread.h"
#include "cbormap.h"
#include "serialcli.h"

// in ms
#define LOOP_CADENCE 1000

#define RF_CS_PIN 10
#define RF_CE_PIN 9
#define RF_IRQ_PIN 2

uint16_t temperatures[8];
uint16_t core_temperature;

static const uint8_t EEMEM rf_link_id = 0;

RFLink *rflink = NULL;
SleepTimer sleepTimer(LOOP_CADENCE);

void CborMessageBuild(void);

void CborMessageBuild(void)
{
    CborMessageInitialize();
    CborMessageAddMap(3);
    CborMapAddSource(CBOR_SOURCE_NANO_TEMPERATURE);
    CborMapAddArray(CBOR_KEY_TEMPERATURE_ARRAY, temperatures, 8);
    CborMapAddCoreTemperature(core_temperature);
}

class GetRFIDCLICommand : public CLICommand
{
    public:
        GetRFIDCLICommand(void) : CLICommand("get_rf_link", 0);
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t rf_id = EEPROM.read(rf_link_id);
                Serial.print("Current RF ID = ");
                Serial.println(rf_id, HEX);
                return 1;
            };
}

class SetRFIDCLICommand : public CLICommand
{
    public:
        SetRFIDCLICommand(void) : CLICommand("set_rf_link", 1);
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t rf_id = (uint8_t)(strtoul(args[0], 0, 16) & 0xFF);
                EEPROM.update(rf_link_id, rf_id);
                Serial.print("New RF ID = ");
                Serial.println(rf_id, HEX);
                return 1;
            };
}


void setup() 
{
    // Setup sleep to idle mode
    SMCR = 0x00;
    
    Serial.begin(115200);

    cli.registerCommand(new GetRFIDCLICommand());
    cli.registerCommand(new SetRFIDCLICommand());

    cli.initialize()

    uint8_t rf_id = EEPROM.read(rf_link_id);

    TemperaturesInitialize();
    rflink = new RFLink(RF_CE_PIN, RF_CS_PIN, RF_IRQ_PIN, rf_id);
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    noInterrupts();
    sleepTimer.enable();

    core_temperature = readAvrTemperature(void);
    TemperaturesPoll(temperatures, 8);

    CborMessageBuild();
    CborMessageBuffer(&buffer, &len);
    if (buffer && len) {
        rflink->send(buffer, len);
    }

    cli.handleInput();

    // Go to sleep, get woken up by the timer
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
