#ifndef CBORMAP_H__
#define CBORMAP_H__

#include <inttypes.h>

typedef enum {
    CBOR_KEY_TIMESTAMP,
    CBOR_KEY_SOURCE,
    CBOR_KEY_VOLTAGE_ARRAY,
    CBOR_KEY_CURRENT_ARRAY,
    CBOR_KEY_POWER_ARRAY,
    CBOR_KEY_TEMPERATURE_ARRAY,
    CBOR_KEY_CORE_TEMPERATURE,
    CBOR_KEY_CBOR_PAYLOAD,
} cborKey_t;

void CborMessageInitialize(void);
void CborMessageAddMap(uint8_t size);
void CborMapAddArray(cborKey_t keyType, void *array, uint8_t itemCount);
void CborMapAddSource(uint8_t source);
void CborMapAddCoreTemperature(int16_t temperature);
void CborMapAddCborPayload(uint8_t *buffer, uint8_t len);
void CborMapAddTimestamp(uint16_t years, uint8_t months, uint8_t days,
                         uint8_t hours, uint8_t minutes, uint8_t seconds);
void CborMessageBuffer(uint8_t **buffer, uint8_t *len);

#endif
// vim:ts=4:sw=4:ai:et:si:sts=4
