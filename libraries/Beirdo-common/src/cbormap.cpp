#include <Arduino.h>
#include <CborEncoder.h>
#include "cbormap.h"

#define CBOR_MAX_LEN 64

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
    cbor_writer = new CborWriter(*cbor_output);
}

void CborMessageAddMap(uint8_t size)
{
    if (!cbor_writer) {
        return;
    }

    cbor_writer->writeMap(size);
}

void CborMapAddArray(cborKey_t keyType, void *array, uint8_t itemCount)
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
        case CBOR_KEY_CURRENT_ARRAY:        // uint32_t containing microamps
            tag = 4;                        // decimal fraction
            exponent = -3;
            break;
        case CBOR_KEY_TEMPERATURE_ARRAY:    // uint16_t containing 1/10 * degC
            tag = 4;                        // decimal fraction
            exponent = -1;
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

void CborMapAddInteger(cborKey_t key, int value)
{
    if (!cbor_writer) {
        return;
    }

    // map key
    cbor_writer->writeInt(key);
    // map value
    cbor_writer->writeInt(value);
}

void CborMapAddLocation(int32_t lat, int32_t lon)
{
    if (!cbor_writer) {
        return;
    }

    // map key
    cbor_writer->writeInt(CBOR_KEY_GPRS_LOCATION);
    // Map value is an array of two decimal fractions
    cbor_writer->writeArray(2);

    cbor_writer->writeTag(4);
    cbor_writer->writeArray(2);
    cbor_writer->writeInt(-6);
    cbor_writer->writeInt(lat);

    cbor_writer->writeTag(4);
    cbor_writer->writeArray(2);
    cbor_writer->writeInt(-6);
    cbor_writer->writeInt(lon);
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

void CborMapAddTimestamp(char *timestr)
{
    if (!cbor_writer) {
        return;
    }

    // Input format (from SIM800):
    //                2003/12/13,18:30:02
    //                012345678901234567890
    // Output format: 2003-12-13T18:30:02Z  (rfc4287#section-3.3)
    timestr[4]  = '-';
    timestr[7]  = '-';
    timestr[10] = 'T';
    timestr[19] = 'Z';
    timestr[20] = '\0';

    // map key
    cbor_writer->writeInt(CBOR_KEY_TIMESTAMP);
    // map value
    cbor_writer->writeTag(0);           // timestamp string
    cbor_writer->writeString(timestr, 20);
}

bool CborMessageBuffer(uint8_t **buffer, uint8_t *len)
{
    if (!cbor_output) {
        *buffer = NULL;
        *len = 0;
        return false;
    }

    uint8_t *outBuf = *buffer;
    uint8_t maxLen = *len;
    
    uint8_t cborLen = cbor_output->getSize();
    uint8_t *cborBuf = NULL;

    if (outBuf) {
        if (cborLen > maxLen) {
            return false;
        }

        cborBuf = cbor_output->getData();
        memcpy(outBuf, cborBuf, cborLen);
        *len = cborLen;
        return true;
    }

    *buffer = cborBuf;
    *len = cborLen;
    return true;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
