#include "serialcli.h"

#include <SPI.h>
#include <LowPower.h>

// in ms
#define LOOP_CADENCE 120
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

#define LED_PWM_PIN PIN_LED_RXL

SerialCLI cli(SerialUSB);

class SetLEDCLICommand : public CLICommand
{
    public:
        SetLEDCLICommand(void) : CLICommand("set_led", 1) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            {
                uint8_t value = (uint8_t)atoi((const char *)args[0]);
                serial()->print("LED value = ");
                serial()->println(value, DEC);
                analogWrite(LED_PWM_PIN, value);
                return 1;
            };
};

void setup() 
{
    cli.registerCommand(new SetLEDCLICommand());
    cli.initialize();

    analogWrite(LED_PWM_PIN, 5);
}

void loop() 
{
    cli.handleInput();
//    LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_ON,
//                  SPI_OFF, USART0_ON, TWI_OFF);
    delay(LOOP_CADENCE);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
