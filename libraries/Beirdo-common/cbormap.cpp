#include <Arduino.h>
#include <CborEncoder.h>
#include "cbormap.h"

#define CBOR_MAX_LEN 64

uint8_t cbor_buffer[CBOR_MAX_LEN];


CborStaticOutput *cbor_output = NULL;
CborWriter *cbor_writer = NULL;

void CborMessageInitialize(void)
{
    if (cbor_writer) {
        delete cbor_writer;
    }
    if (cbor_output) {
        delete cbor_output;
    }
    cbor_output = new CborStaticOutput(CBOR_MAX_LEN);
    cbor_writer = new CborWriter(*output);
}

void CborMessageAddMap(uint8_t size)
{
    if (!cbor_writer) {
        return;
    }

    cbor_writer->writeMap(size);
}

void CborMapAddArray(cborKey_t keyType, void *array, uint8 itemCount)
{
    uint8_t tag;
    int8_t exponent;
    int value;

    if (!cbor_writer) {
        return;
    }

    switch (keyType) {
        case CBOR_KEY_VOLTAGE_ARRAY:        // uint32_t containing millivolts
        case CBOR_KEY_POWER_ARRAY:          // uint32_t containing milliwatts
            tag = 4;                        // decimal fraction
            exponent = -3;
            break;
        case CBOR_KEY_CURRENT_ARRAY:        // uint32_t containing microamps
            tag = 4;                        // decimal fraction
            exponent = -6;
            break;
        case CBOR_KEY_TEMPERATURE_ARRAY:    // uint16_t containing 1/128 * degC
            tag = 5;                        // bigfloat (binary fraction)
            exponent = -7;
            break;
        default:
            return;
    }

    // Map key
    cbor_writer->writeInt(keyType);
    // Map value is an array
    cbor_writer->writeArray(itemCount);

    // Array items
    for (int i = 0; i < itemCount; i++) {
        cbor_writer->writeTag(tag);
        cbor_writer->writeArray(2);
        cbor_writer->writeInt(exponent);
        switch (keyType) {
            case CBOR_KEY_VOLTAGE_ARRAY:
            case CBOR_KEY_POWER_ARRAY:
            case CBOR_KEY_CURRENT_ARRAY:
                value = ((uint32_t *)array)[i];
                break;
            case CBOR_KEY_TEMPERATURE_ARRAY:
                value = ((uint16_t *)array)[i];
                break;
            default:
                return;
        }
        cbor_writer->writeInt(value);
    }
}

void CborMapAddSource(uint8_t source)
{
    if (!cbor_writer) {
        return;
    }

    // map key
    cbor_writer->writeInt(CBOR_KEY_SOURCE);
    // map value
    cbor_writer->writeInt(source);
}

void CborMapAddCoreTemperature(int16_t temperature)
{
    if (!cbor_writer) {
        return;
    }

    // map key
    cbor_writer->writeInt(CBOR_KEY_CORE_TEMPERATURE);
    // map value
    cbor_writer->writeTag(4);       // decimal fraction (1/10 degC)
    cbor_writer->writeArray(2);
    cbor_writer->writeInt(-1);      // exponent
    cbor_writer->writeInt(temperature);
}

void CborMapAddCborPayload(uint8_t *buffer, uint8_t len)
{
    if (!cbor_writer) {
        return;
    }

    // map key
    cbor_writer->writeInt(CBOR_KEY_CBOR_PAYLOAD);
    // map value
    cbor_writer->writeTag(24);       // encoded cbor data item
    cbor_writer->writeBytes(buffer, len);
}

uint8_t timestamp[21];
void sprintInt(uint8_t *buffer, uint16_t value, uint8_t digits);

void sprintInt(uint8_t *buffer, uint16_t value, uint8_t digits)
{
    if (!buffer) {
        return;
    }

    for (buffer += digits - 1; digits; digits--, buffer--)
    {
        *buffer = 0x30 + (value % 10);
        value /= 10;
    }
}

void CborMapAddTimestamp(uint16_t years, uint8_t months, uint8_t days,
                         uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    if (!cbor_writer) {
        return;
    }

    //                012345678901234567890
    // Output format: 2003-12-13T18:30:02Z  (rfc4287#section-3.3)
    sprintInt(&timestamp[0], years, 4);
    timestamp[4] = '-';
    sprintInt(&timestamp[5], months, 2);
    timestamp[7] = '-';
    sprintInt(&timestamp[8], days, 2);
    timestamp[10] = 'T';
    sprintInt(&timestamp[11], hours, 2);
    timestamp[13] = ':';
    sprintInt(&timestamp[14], minutes, 2);
    timestamp[16] = ':';
    sprintInt(&timestamp[17], seconds, 2);
    timestamp[19] = 'Z';
    timestamp[20] = '\0';

    // map key
    cbor_writer->writeInt(CBOR_KEY_CBOR_TIMESTAMP);
    // map value
    cbor_writer->writeTag(0);           // timestamp string
    cbor_writer->writeString(timestamp, 20);
}

void CborMessageBuffer(uint8_t **buffer, uint8_t *len)
{
    if (!cbor_output) {
        *buffer = NULL;
        *len = 0;
        return;
    }

    *buffer = cbor_output->getData();
    *len = cbor_output->getSize();
}

// vim:ts=4:sw=4:ai:et:si:sts=4
