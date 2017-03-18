#include <twimaster.h>

// SMBus Hacker v1.0 by FalconFour
//  with huge credit to liudr (http://liudr.wordpress.com/) for the awesome work on phi_prompt
//
// insert standard block of legalese and disclaimers here; by relation of the phi_prompt license,
// this code is only licensed for personal and educational use. don't blow yourself up.
// don't sell it. don't blame me if you blow yourself up. and don't sue me if you need more
// money, because i don't have any! =P
//
// The battery SMBus connection always connects to analog pins 5 and 6. To say that SCL is analog
//  5 and SDA is analog 4 probably wouldn't do much good since they don't usually mark that stuff
//  on batteries. First, start with a multimeter and a length of wire, checking the outer pins
//  first. Those are usually + and -. You need "-", unplug + when you find it. Poke around with
//  the "+" wire you just removed, and ground still plugged in. You should NOT see over 5 volts on
//  any pins. Avoid a pin if you see more than 5 volts. Use the SMBus Test function to test your
//  wiring. It won't respond to anything but its own address, so you might want to use a SMBus
//  Scan when you're not having luck but have secure wires.
//
//                          --- THIS IS WHERE I GET YOUR ATTENTION! ---
//    These are the parts you need to modify to your own setup. Add buttons as needed (minimum of
//    4 to navigate without resetting every mis-step), change the LCD pins, mode, etc.
//

byte deviceAddress = 0x0B; // most battery controllers seem to use this
byte cmdSet = 0; // default command set (bq2040), seems to have good luck

// -- Okay, you can hit Upload now.
//
void fmtDouble(double val, byte precision, char *buf, unsigned bufLen = 0xffff);
unsigned fmtUnsigned(unsigned long val, char *buf, unsigned bufLen = 0xffff, byte width = 0);
void DisplaySingleCommand(uint8_t cmdNum);

#define bufferLen 32
char i2cBuffer[bufferLen];
uint8_t serialCommand;
unsigned int serialData;

#define BATT_MAH 0
#define BATT_MA 1
#define BATT_MV 2
#define BATT_MINUTES 3
#define BATT_PERCENT 4
#define BATT_TENTH_K 5
#define BATT_BITFIELD 6
#define BATT_DEC 7
#define BATT_HEX 8
#define BATT_STRING 16

#define Cmd_ManufacturerAccess     0x00
#define Cmd_RemainingCapacityAlarm 0x01
#define Cmd_RemainingTimeAlarm     0x02
#define Cmd_BatteryMode            0x03
#define Cmd_AtRate                 0x04
#define Cmd_AtRateTimeToFull       0x05
#define Cmd_AtRateTimeToEmpty      0x06
#define Cmd_AtRateOK               0x07
#define Cmd_Temperature            0x08
#define Cmd_Voltage                0x09
#define Cmd_Current                0x0A
#define Cmd_AverageCurrent         0x0B
#define Cmd_MaxError               0x0C
#define Cmd_RelativeStateOfCharge  0x0D
#define Cmd_AbsoluteStateOfCharge  0x0E
#define Cmd_RemainingCapacity      0x0F
#define Cmd_FullChargeCapacity     0x10
#define Cmd_RunTimeToEmpty         0x11
#define Cmd_AverageTimeToEmpty     0x12
#define Cmd_AverageTimeToFull      0x13
#define Cmd_ChargingCurrent        0x14
#define Cmd_ChargingVoltage        0x15
#define Cmd_BatteryStatus          0x16
#define Cmd_CycleCount             0x17
#define Cmd_DesignCapacity         0x18
#define Cmd_DesignVoltage          0x19
#define Cmd_SpecificationInfo      0x1A
#define Cmd_ManufactureDate        0x1B
#define Cmd_SerialNumber           0x1C
#define Cmd_ManufacturerName       0x1D
#define Cmd_DeviceName             0x1E
#define Cmd_DeviceChemistry        0x1F
#define Cmd_ManufacturerData       0x20
#define Cmd_Flags                  0x21
#define Cmd_EDV1                   0x22
#define Cmd_EDVF                   0x23

// table of ALL global command labels used in the command sets; if you want to add a new command, tack it onto the bottom.
const char cmdLabel_ManufacturerAccess[] PROGMEM = "ManufacturerAccess";
const char cmdLabel_RemainingCapacityAlarm[] PROGMEM = "RemainingCapacityAlarm";
const char cmdLabel_RemainingTimeAlarm[] PROGMEM = "RemainingTimeAlarm";
const char cmdLabel_BatteryMode[] PROGMEM = "BatteryMode";
const char cmdLabel_AtRate[] PROGMEM = "AtRate";
const char cmdLabel_AtRateTimeToFull[] PROGMEM = "AtRateTimeToFull";
const char cmdLabel_AtRateTimeToEmpty[] PROGMEM = "AtRateTimeToEmpty";
const char cmdLabel_AtRateOK[] PROGMEM = "AtRateOK";
const char cmdLabel_Temperature[] PROGMEM = "Temperature";
const char cmdLabel_Voltage[] PROGMEM = "Voltage";
const char cmdLabel_Current[] PROGMEM = "Current";
const char cmdLabel_AverageCurrent[] PROGMEM = "AverageCurrent";
const char cmdLabel_MaxError[] PROGMEM = "MaxError";
const char cmdLabel_RelativeStateOfCharge[] PROGMEM = "RelativeStateOfCharge";
const char cmdLabel_AbsoluteStateOfCharge[] PROGMEM = "AbsoluteStateOfCharge";
const char cmdLabel_RemainingCapacity[] PROGMEM = "RemainingCapacity";
const char cmdLabel_FullChargeCapacity[] PROGMEM = "FullChargeCapacity";
const char cmdLabel_RunTimeToEmpty[] PROGMEM = "RunTimeToEmpty";
const char cmdLabel_AverageTimeToEmpty[] PROGMEM = "AverageTimeToEmpty";
const char cmdLabel_AverageTimeToFull[] PROGMEM = "AverageTimeToFull";
const char cmdLabel_ChargingCurrent[] PROGMEM = "ChargingCurrent";
const char cmdLabel_ChargingVoltage[] PROGMEM = "ChargingVoltage";
const char cmdLabel_BatteryStatus[] PROGMEM = "BatteryStatus";
const char cmdLabel_CycleCount[] PROGMEM = "CycleCount";
const char cmdLabel_DesignCapacity[] PROGMEM = "DesignCapacity";
const char cmdLabel_DesignVoltage[] PROGMEM = "DesignVoltage";
const char cmdLabel_SpecificationInfo[] PROGMEM = "SpecificationInfo";
const char cmdLabel_ManufactureDate[] PROGMEM = "ManufactureDate";
const char cmdLabel_SerialNumber[] PROGMEM = "SerialNumber";
const char cmdLabel_ManufacturerName[] PROGMEM = "ManufacturerName";
const char cmdLabel_DeviceName[] PROGMEM = "DeviceName";
const char cmdLabel_DeviceChemistry[] PROGMEM = "DeviceChemistry";
const char cmdLabel_ManufacturerData[] PROGMEM = "ManufacturerData";
const char cmdLabel_Flags[] PROGMEM = "Flags";
const char cmdLabel_EDV1[] PROGMEM = "EDV1";
const char cmdLabel_EDVF[] PROGMEM = "EDVF";

// this is set up in the same fashion as phi_prompt menus, because it's also used as one in the selector. String the names of your commands together IN ORDER with the commands definition below this.
const char *bq2040Labels[] =        { cmdLabel_ManufacturerAccess, cmdLabel_RemainingCapacityAlarm, cmdLabel_RemainingTimeAlarm, cmdLabel_BatteryMode, cmdLabel_AtRate, cmdLabel_AtRateTimeToFull, cmdLabel_AtRateTimeToEmpty, cmdLabel_AtRateOK, cmdLabel_Temperature, cmdLabel_Voltage, cmdLabel_Current, cmdLabel_AverageCurrent, cmdLabel_MaxError, cmdLabel_RelativeStateOfCharge, cmdLabel_AbsoluteStateOfCharge, cmdLabel_RemainingCapacity, cmdLabel_FullChargeCapacity, cmdLabel_RunTimeToEmpty, cmdLabel_AverageTimeToEmpty, cmdLabel_AverageTimeToFull, cmdLabel_ChargingCurrent, cmdLabel_ChargingVoltage, cmdLabel_BatteryStatus, cmdLabel_CycleCount, cmdLabel_DesignCapacity, cmdLabel_DesignVoltage, cmdLabel_SpecificationInfo, cmdLabel_ManufactureDate, cmdLabel_SerialNumber, cmdLabel_ManufacturerName, cmdLabel_DeviceName, cmdLabel_DeviceChemistry, cmdLabel_ManufacturerData, cmdLabel_Flags, cmdLabel_EDV1, cmdLabel_EDVF };
// here, a two-dimension array. first byte is the command code itself (hex). second byte is the type of result it's expected to return (all except the strings come through as words.
const uint8_t bq2040Commands[][2] PROGMEM = {      { 0x00, BATT_HEX },          { 0x01, BATT_MAH },            { 0x02, BATT_MINUTES }, { 0x03, BATT_BITFIELD },{ 0x04, BATT_MA }, { 0x05, BATT_MINUTES },    { 0x06, BATT_MINUTES },   {0x07, BATT_HEX},  {0x08, BATT_TENTH_K}, {0x09, BATT_MV}, { 0x0A, BATT_MA },   { 0x0B, BATT_MA },   {0x0C, BATT_PERCENT},    { 0x0D, BATT_PERCENT },         { 0x0E, BATT_PERCENT },         { 0x0F, BATT_MAH },         { 0x10, BATT_MAH },      { 0x11, BATT_MINUTES },     { 0x12, BATT_MINUTES },     { 0x13, BATT_MINUTES },      { 0x14, BATT_MA },        { 0x15, BATT_MV },    { 0x16, BATT_BITFIELD }, { 0x17, BATT_DEC },    { 0x18, BATT_MAH },      { 0x19, BATT_MV },     { 0x1A, BATT_BITFIELD },   { 0x1B, BATT_BITFIELD },   { 0x1C, BATT_DEC },    { 0x20, BATT_STRING },    {0x21, BATT_STRING},  { 0x22, BATT_STRING },     { 0x23, BATT_STRING },{0x2F, BATT_BITFIELD},{0x3E, BATT_MV},{0x3F, BATT_MV} };

// update each of these commands with your custom command set, just add a new "case" correlating to the cmdset_items[] position above.
uint8_t cmd_getCode(uint8_t command) {
  switch (cmdSet) {
    case 0:
      // bq2040 commands
      return pgm_read_byte(&bq2040Commands[command][0]); // command is first parameter
      break;
  }
}
uint8_t cmd_getType(uint8_t command) {
  switch (cmdSet) {
    case 0:
      // bq2040 commands
      return pgm_read_byte(&bq2040Commands[command][1]); // type is second parameter
      break;
  }
}
void cmd_getLabel(uint8_t command, char* destBuffer) {
  switch (cmdSet) {
    case 0:
      // bq2040 commands
      strlcpy_P(destBuffer, (char *)bq2040Labels[command], bufferLen-1);
      break;
  }
}
char** cmd_getPtr() {
  switch (cmdSet) {
    case 0:
      return (char**)&bq2040Labels;
      break;
  }
}
uint8_t cmd_getLength() {
  switch (cmdSet) {
    case 0:
      return (sizeof(bq2040Labels) / sizeof(&bq2040Labels));
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("BattMon 1.0 ");
  Serial.println("for Arduino ");
  
  i2c_init();
  PORTC = (1 << PORTC4) | (1 << PORTC5); //enable pullups


  uint8_t start = 0x00;
  uint8_t stop = cmd_getLength();
  uint8_t i;

  for (i = start; i < stop; i++) {
    DisplaySingleCommand(i);
  }

  pinMode(13, OUTPUT);
}

void loop() {
    // Nothing to do here, so we'll just blink the built-in LED
    digitalWrite(13,HIGH);
    delay(300);
    digitalWrite(13,LOW);
    delay(300);
}


void DisplaySingleCommand(uint8_t cmdNum) {
  int wordBuffer;
  double valueBuffer;

  Serial.print("(");
  Serial.print(cmdNum, HEX);
  Serial.print(") ");

  uint8_t cmdType = cmd_getType(cmdNum);
  uint8_t cmdCode = cmd_getCode(cmdNum);

  cmd_getLabel(cmdNum, i2cBuffer);
  Serial.print(i2cBuffer);
  Serial.print(": ");

  if (cmdType < BATT_STRING) {
    wordBuffer = i2c_smbus_read_word(cmdCode);
  } else if (cmdType == BATT_STRING) {
    i2c_smbus_read_block(cmdCode, i2cBuffer, bufferLen);
  } else return;

  switch (cmdType) {
    case BATT_MAH:
      valueBuffer = wordBuffer/1000;
      fmtDouble(valueBuffer,6,i2cBuffer,bufferLen);
      strcpy_P(i2cBuffer+strcspn(i2cBuffer,0),PSTR(" Ah"));
      break;
    case BATT_MA:
      valueBuffer = wordBuffer/1000;
      fmtDouble(valueBuffer,6,i2cBuffer,bufferLen);
      strcpy_P(i2cBuffer+strcspn(i2cBuffer,0),PSTR(" Amps"));
      break;
    case BATT_MV:
      valueBuffer = wordBuffer/1000;
      fmtDouble(valueBuffer,6,i2cBuffer,bufferLen);
      strcpy_P(i2cBuffer+strcspn(i2cBuffer,0),PSTR(" Volts"));
      break;
    case BATT_MINUTES:
      itoa(wordBuffer,i2cBuffer,10);
      strcpy_P(i2cBuffer+strcspn(i2cBuffer,0),PSTR(" Minutes"));
      break;
    case BATT_PERCENT:
      itoa(wordBuffer,i2cBuffer,10);
      strcpy_P(i2cBuffer+strcspn(i2cBuffer,0),PSTR("%"));
      break;
    case BATT_TENTH_K:
      valueBuffer = ((float)wordBuffer/10 - 273.15) * 1.8 + 32;
      fmtDouble(valueBuffer,6,i2cBuffer,bufferLen);
      strcpy_P(i2cBuffer+strcspn(i2cBuffer,0),PSTR(" F"));
      break;
    case BATT_BITFIELD:
      itoa(wordBuffer,i2cBuffer,2);
      break;
    case BATT_DEC:
      itoa(wordBuffer,i2cBuffer,10);
      break;
    case BATT_HEX:
      strcpy_P(i2cBuffer,PSTR("0x"));
      itoa(wordBuffer,i2cBuffer+2,16);
      break;
  }

  Serial.println(i2cBuffer);

}

void i2c_smbus_write_word ( uint8_t command, unsigned int data ) {
  i2c_start((deviceAddress<<1) + I2C_WRITE);
  i2c_write(command);
  i2c_write((uint8_t)data);
  i2c_write((uint8_t)(data>>8));
  i2c_stop();
  return;
}

unsigned int i2c_smbus_read_word ( uint8_t command ) {
  unsigned int buffer = 0;
  i2c_start((deviceAddress<<1) + I2C_WRITE);
  i2c_write(command);
  i2c_rep_start((deviceAddress<<1) + I2C_READ);
  buffer = i2c_readAck();
  buffer += i2c_readNak() << 8;
  i2c_stop();
  return buffer;
}

uint8_t i2c_smbus_read_block ( uint8_t command, char* blockBuffer, uint8_t blockBufferLen ) {
  uint8_t x, num_bytes;
  i2c_start((deviceAddress<<1) + I2C_WRITE);
  i2c_write(command);
  i2c_rep_start((deviceAddress<<1) + I2C_READ);
  num_bytes = i2c_readAck(); // num of bytes; 1 byte will be index 0
  num_bytes = constrain(num_bytes,0,blockBufferLen-2); // room for null at the end
  for (x=0; x<num_bytes-1; x++) { // -1 because x=num_bytes-1 if x<y; last byte needs to be "nack"'d, x<y-1
    blockBuffer[x] = i2c_readAck();
  }
  blockBuffer[x++] = i2c_readNak(); // this will nack the last byte and store it in x's num_bytes-1 address.
  blockBuffer[x] = 0; // and null it at last_byte+1 for LCD printing
  i2c_stop();
  return num_bytes;
}


//
// Produce a formatted string in a buffer corresponding to the value provided.
// If the 'width' parameter is non-zero, the value will be padded with leading
// zeroes to achieve the specified width.  The number of characters added to
// the buffer (not including the null termination) is returned.
//
unsigned fmtUnsigned(unsigned long val, char *buf, unsigned bufLen, byte width) {
  if (!buf || !bufLen) return(0);

  // produce the digit string (backwards in the digit buffer)
  char dbuf[10];
  unsigned idx = 0;
  while (idx < sizeof(dbuf)) {
    dbuf[idx++] = (val % 10) + '0';
    if ((val /= 10) == 0) break;
  }

  // copy the optional leading zeroes and digits to the target buffer
  unsigned len = 0;
  byte padding = (width > idx) ? width - idx : 0;
  char c = '0';
  while ((--bufLen > 0) && (idx || padding)) {
    if (padding) padding--;
    else c = dbuf[--idx];
    *buf++ = c;
    len++;
  }

  // add the null termination
  *buf = '\0';
  return(len);
}

//
// Format a floating point value with number of decimal places.
// The 'precision' parameter is a number from 0 to 6 indicating the desired decimal places.
// The 'buf' parameter points to a buffer to receive the formatted string.  This must be
// sufficiently large to contain the resulting string.  The buffer's length may be
// optionally specified.  If it is given, the maximum length of the generated string
// will be one less than the specified value.
//
// example: fmtDouble(3.1415, 2, buf); // produces 3.14 (two decimal places)
//
void fmtDouble(double val, byte precision, char *buf, unsigned bufLen) {
  if (!buf || !bufLen) return;
  // limit the precision to the maximum allowed value
  const byte maxPrecision = 6;
  if (precision > maxPrecision)
    precision = maxPrecision;

  if (--bufLen > 0)
  {
    // check for a negative value
    if (val < 0.0) {
      val = -val;
      *buf = '-';
      bufLen--;
    }

    // compute the rounding factor and fractional multiplier
    double roundingFactor = 0.5;
    unsigned long mult = 1;
    for (byte i = 0; i < precision; i++) {
      roundingFactor /= 10.0;
      mult *= 10;
    }
    if (bufLen > 0) {
      // apply the rounding factor
      val += roundingFactor;

      // add the integral portion to the buffer
      unsigned len = fmtUnsigned((unsigned long)val, buf, bufLen);
      buf += len;
      bufLen -= len;
    }

    // handle the fractional portion
    if ((precision > 0) && (bufLen > 0)) {
      *buf++ = '.';
      if (--bufLen > 0) buf += fmtUnsigned((unsigned long)((val - (unsigned long)val) * mult), buf, bufLen, precision);
    }
  }

  // null-terminate the string
  *buf = '\0';
}

