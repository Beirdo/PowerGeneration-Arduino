#include <LowPower.h>
#include <Adafruit_FRAM_SPI.h>
#include <Adafruit_GFX.h>
#include <SSD1306.h>

#include "adcread.h"
#include "serialcli.h"
#include "lcdscreen.h"
#include "utils.h"

// in ms
#define LOOP_CADENCE 120
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

// Thresholds for turning on/off the DC/DC converter (mV)
#define THRESH_ON 900
#define THRESH_OFF 800

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix SSD1306.h!");
#endif

void coulombISR(void);
void newbattISR(void);

SerialCLI CLI(Serial);

ADCRead adcread;
int16_t core_temperature;
int32_t battery_voltage = 0;
int32_t supercap_voltage = 0;

uint8_t shutdown;

volatile int32_t supercap_charge_uC = 0;
volatile int32_t output_charge_uC = 0;
volatile int32_t supercap_charge_uAh = 0;
volatile int32_t output_charge_uAh = 0;
volatile int32_t supercap_charge_uWh = 0;
volatile int32_t output_charge_uWh = 0;

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
                     (void *)&battery_voltage, formatAutoScaleMilli, "V"));
    lcdDeck.addFrame(new LCDScreen("Vcc",
                     (void *)&supercap_voltage, formatAutoScaleMilli, "V"));
    lcdDeck.addFrame(new LCDScreen("SuperCap", (void *)&supercap_charge_uC,
                     formatAutoScaleMicro, "C"));
    lcdDeck.addFrame(new LCDScreen("Output", (void *)&output_charge_uC,
                     formatAutoScaleMicro, "C"));
    lcdDeck.addFrame(new LCDScreen("SuperCap", (void *)&supercap_charge_uAh,
                     formatAutoScaleMicro, "Ah"));
    lcdDeck.addFrame(new LCDScreen("Output", (void *)&output_charge_uAh,
                     formatAutoScaleMicro, "Ah"));
    lcdDeck.addFrame(new LCDScreen("SuperCap", (void *)&supercap_charge_uWh,
                     formatAutoScaleMicro, "Wh"));
    lcdDeck.addFrame(new LCDScreen("Output", (void *)&output_charge_uWh,
                     formatAutoScaleMicro, "Wh"));


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

    supercap_voltage = adcread.readVcc();
    battery_voltage  = adcread.mapPin(VBATT_ADC_PIN, 0, 5000);

    if (shutdown && battery_voltage >= THRESH_ON) {
        shutdown = 0;
        digitalWrite(COUNT_SHDN_PIN, HIGH);
    } else if (!shutdown && battery_voltage <= THRESH_OFF) {
        shutdown = 1;
        digitalWrite(COUNT_SHDN_PIN, LOW);
    }

    lcdTicks++;

    if (lcdTicks >= SWAP_COUNT) {
        lcdTicks -= SWAP_COUNT;

        core_temperature = adcread.readCoreTemperature();
        lcdDeck.setBatteryLevel(supercap_voltage, 2700, 5000);
        
        lcdIndex = lcdDeck.nextIndex();
        lcdDeck.formatFrame(lcdIndex);
        lcdDeck.displayFrame();
    }

    CLI.handleInput();

    LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                  SPI_OFF, USART0_ON, TWI_OFF);
}

// 1000000 * 1 / (Gvf * Rsense)
// = 1000000 / (32.55 * 0.110)
// = (1000000 * 100 * 100) / (3255 * 11)
#define MICRO_COULOMB_PER_IRQ ((1000000L * 1000L) / (3255 * 11) * 10)

void coulombISR(void)
{
    uint8_t pol = digitalRead(COUNT_POL_PIN);
    volatile int32_t old_output_uAh = output_charge_uAh;

    if (pol) {
        // High is charging the capacitors, assume neglible power going to output
        supercap_charge_uC += MICRO_COULOMB_PER_IRQ;
    } else {
        // Low is discharging the capacitor, assume all power going to output
        supercap_charge_uC = max(0, supercap_charge_uC - MICRO_COULOMB_PER_IRQ);
        output_charge_uC += MICRO_COULOMB_PER_IRQ;
    }

    // Convert from x [uC] to y [uAh]
    supercap_charge_uAh = supercap_charge_uC / 3600;
    output_charge_uAh = output_charge_uC / 3600;

    // Convert from x [uAh] @ y [mV] to z [uWh]
    // 1 [uAh] * 1 [mV] = 1 [nWh]
    // 1000 [mAh] * 1 [mV] = 1000 [uWh]
    // 1 [mAh] * 1 [mV] = 1 [uWh]

    if (supercap_charge_uAh >= 1000000) {
        supercap_charge_uWh = (supercap_charge_uAh / 1000) * supercap_voltage;
    } else {
        supercap_charge_uWh = (supercap_charge_uAh * supercap_voltage) / 1000;
    }

    if (output_charge_uAh >= 1000000) {
        output_charge_uWh += ((output_charge_uAh - old_output_uAh) / 1000) * supercap_voltage;
    } else {
        output_charge_uWh += ((output_charge_uAh - old_output_uAh) * supercap_voltage) / 1000;
    }
}

void newbattISR(void)
{
    // Reset the output charge value to 0
    output_charge_uC = 0;
    output_charge_uAh = 0;
    output_charge_uWh = 0;
}

// vim:ts=4:sw=4:ai:et:si:sts=4

