#ifndef CBORMAP_H__
#define CBORMAP_H__

#include <Arduino.h>
#include <inttypes.h>

typedef enum {
    CBOR_KEY_TIMESTAMP,
    CBOR_KEY_SOURCE,
    CBOR_KEY_RF_ID,
    CBOR_KEY_VOLTAGE_ARRAY,
    CBOR_KEY_CURRENT_ARRAY,
    CBOR_KEY_POWER_ARRAY,
    CBOR_KEY_TEMPERATURE_ARRAY,
    CBOR_KEY_CORE_TEMPERATURE,
    CBOR_KEY_GPRS_RSSI,
    CBOR_KEY_GPRS_LOCATION,
    CBOR_KEY_CBOR_PAYLOAD,
} cborKey_t;

typedef enum {
    CBOR_SOURCE_CHARGER,
    CBOR_SOURCE_MPPT,
    CBOR_SOURCE_NANO_TEMPERATURE,
    CBOR_SOURCE_NANO_GPRS,
} cborSource_t;

void CborMessageInitialize(void);
void CborMessageAddMap(uint8_t size);
void CborMapAddArray(cborKey_t keyType, void *array, uint8_t itemCount);
void CborMapAddInteger(cborKey_t key, int value);
void CborMapAddCborPayload(uint8_t *buffer, uint8_t len);
void CborMapAddLocation(float lat, float lon);
void CborMapAddTimestamp(char *timestr);
bool CborMessageBuffer(uint8_t **buffer, uint8_t *len);

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
