#include <LowPower.h>
#include <Adafruit_FRAM_SPI.h>
#include <Adafruit_GFX.h>
#include <SSD1306.h>

#include "adcread.h"
#include "serialcli.h"
#include "lcdscreen.h"
#include "eeprom.h"
#include "utils.h"

// in ms
#define LOOP_CADENCE 1000
#define SWAP_TIME 2000
#define SWAP_COUNT (SWAP_TIME / LOOP_CADENCE)

uint16_t lcdTicks;
int8_t lcdIndex;

#define VBATT_ADC_PIN 7

#define FRAM_CS_PIN 10

#define COUNT_SHDN_PIN 9
#define COUNT_POL_PIN 8
#define COUNT_INT_PIN 2

#define NEWBATT_PIN 3

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix SSD1306.h!");
#endif

void coulombISR(void);
void newbattISR(void);

SerialCLI CLI(Serial);

ADCRead adcread;
int16_t core_temperature;
uint32_t battery_voltage;
uint32_t supercap_voltage;

uint8_t shutdown;

volatile uint32_t supercap_charge_q; 
volatile uint32_t liion_charge_q; 
volatile uint32_t supercap_charge_mAh; 
volatile uint32_t liion_charge_mAh; 

Adafruit_FRAM_SPI fram(FRAM_CS_PIN);
SSD1306 oled;
LCDDeck lcdDeck(&oled);


class InitializeLogoCLICommand : public CLICommand
{
    public:
        InitializeLogoCLICommand(void) : CLICommand("initlogo", 0) {};
        uint8_t run(uint8_t nargs, uint8_t **args)
            { 
                CLI.serial()->println("Writing logo to FRAM");
                oled.initializeLogo();
                CLI.serial()->println("Done");
                return 1;
            };
};


void setup() 
{
    CLI.registerCommand(new InitializeLogoCLICommand());
    CLI.initialize();

    bool framInit = fram.begin();
    if (!framInit) {
        CLI.serial()->println("Can't find attached FRAM");
    }
    
    oled.begin(SSD1306_SWITCHCAPVCC);
    if (framInit) {
        oled.attachRAM(&fram, 0x0000, 0x0400);
    }
    oled.display();

    lcdDeck.addFrame(new LCDScreen("Core Temp",
                     (void *)&core_temperature, formatTemperature, "C"));
    lcdDeck.addFrame(new LCDScreen("Battery",
                     (void *)&battery_voltage, formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Vcc",
                     (void *)&supercap_voltage, formatAutoScale, "V"));
    lcdDeck.addFrame(new LCDScreen("Cap Charge", (void *)&supercap_charge_q,
                     formatAutoScale, "C"));
    lcdDeck.addFrame(new LCDScreen("LiIon Charge", (void *)&liion_charge_q,
                     formatAutoScale, "C"));
    lcdDeck.addFrame(new LCDScreen("Cap Charge", (void *)&supercap_charge_mAh,
                     formatAutoScale, "Ah"));
    lcdDeck.addFrame(new LCDScreen("LiIon Charge", (void *)&liion_charge_mAh,
                     formatAutoScale, "Ah"));

    lcdTicks = 0;

    pinMode(COUNT_POL_PIN, INPUT);
    pinMode(COUNT_INT_PIN, INPUT);
    pinMode(NEWBATT_PIN, INPUT);

    shutdown = 1;
    digitalWrite(COUNT_SHDN_PIN, LOW);
    pinMode(COUNT_SHDN_PIN, OUTPUT);
    
    attachInterrupt(digitalPinToInterrupt(COUNT_INT_PIN), coulombISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(NEWBATT_PIN), newbattISR, FALLING);
}

void loop() 
{
    uint8_t *buffer;
    uint8_t len;

    lcdTicks++;
    if (lcdTicks >= SWAP_COUNT) {
        lcdTicks -= SWAP_COUNT;

        core_temperature = adcread.readCoreTemperature();
        supercap_voltage = adcread.readVcc();
        battery_voltage  = adcread.mapPin(VBATT_ADC_PIN, 0, 5000);

        if (shutdown && battery_voltage >= 900) {
            shutdown = 0;
            digitalWrite(COUNT_SHDN_PIN, HIGH);
        } else if (!shutdown && battery_voltage <= 800) {
            shutdown = 1;
            digitalWrite(COUNT_SHDN_PIN, LOW);
        }

        lcdIndex = lcdDeck.nextIndex();
        lcdDeck.formatFrame(lcdIndex);
        lcdDeck.displayFrame();
    }

    CLI.handleInput();

    LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                  SPI_OFF, USART0_ON, TWI_OFF);
}

// 1000 * 1 / (Gvf * Rsense)
// = 1000 / (32.55 * 0.110)
// = (1000 * 1000 * 1000) / (32550 * 110)
#define MILLI_COULOMB_PER_IRQ (1000000000L / (32550 * 110))
void coulombISR(void)
{
    pol = digitalRead(COUNT_POL_PIN);
    if (pol) {
        // High is charging the capacitors
        supercap_charge_q += MILLI_COULOMB_PER_IRQ;
    } else {
        // Low is discharging the capacitor, assume all power going to Li-Ion
        supercap_charge_q = max(0, supercap_charge_q - MILLI_COULOMB_PER_IRQ);
        liion_charge_q += MILLI_COULOMB_PER_IRQ;
    }

    // Convert from millicoulomb to mAh
    supercap_charge_mAh = supercap_charge_q / 3600;
    liion_charge_mAh = liion_charge_q / 3600;
}

void newbattISR(void)
{
    // Reset the LiIon charge value to 0
    liion_charge_q = 0;
    liion_charge_mAh = 0;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
