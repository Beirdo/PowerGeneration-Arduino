#include <string.h>
#include <SIM800.h>
#include "cbormap.h"

// Ting's default
#define DEFAULT_APN "wholesale"
#define DEFAULT_URL "https://apigatewayurl/method"

GPRS::GPRS(int8_t reset_pin, int8_t enable_pin, int8_t dtr_pin)
{
    m_gprs = new CGPRS_SIM800(&Serial, NULL, reset_pin, enable_pin, dtr_pin);
    m_reset_pin = reset_pin;
    m_enable_pin = enable_pin;
    m_dtr_pin = dtr_pin;
    m_state = GPRS_DISABLED;
    m_apn = DEFAULT_APN;
    m_url = DEFAULT_URL;
    m_buflen = 0;
    memset(&m_location, 0x00, sizeof(GSM_LOCATION));
    m_epochTime = 0;
    m_lastEpochTime = 0;
}

bool GPRS::isDisabled(void)
{
    if (m_state != GPRS_DISABLED && m_state != GPRS_HTTPS_DONE) {
        return false;
    }
    if (m_enable_pin == -1) {
        return false;
    }

    // if the enable jumper is removed, disabling the GPRS module, this will
    // be high when read.  If the enable jumper is installed, allowing the
    // GPRS module to be enabled, this will be low when read.
    pinMode(m_enable_pin, INPUT);
    return (digitalRead(m_enable_pin) == HIGH);
}

void GPRS::stateMachine(void)
{
    gprs_state_t next_state = m_state;

    switch (m_state) {
        case GPRS_DISABLED:
            if (!isDisabled()) {
                next_state = GPRS_INIT;
            }
            break;
        case GPRS_INIT:
            if (m_gprs->init()) {
                next_state = GPRS_SETUP;
            }
            break;
        case GPRS_SETUP:
            if (m_gprs->setup(m_apn) == 0) {
                next_state = GPRS_READY;
            } else {
                strncpy(m_error, m_gprs->buffer(), MAX_ERROR_LEN);
                next_state = GPRS_DISABLED;
            }
            break;
        case GPRS_READY:
            delay(3000);
            next_state = GPRS_HTTPS_INIT;
            break;
        case GPRS_HTTPS_INIT:
            if (m_gprs->httpsInit()) {
                delay(3000);
                next_state = GPRS_HTTPS_READY;
            } else {
                strncpy(m_error, m_gprs->buffer(), MAX_ERROR_LEN);
                m_gprs->httpsUninit();
                delay(1000);
            }
            break;
        case GPRS_HTTPS_READY:
            if (m_buflen != 0) {
                next_state = GPRS_HTTPS_CONNECT;
            } else if (m_nextBuflen != 0) {
                send_buffer();
                next_state = GPRS_HTTPS_CONNECT;
            }
            break;
        case GPRS_HTTPS_CONNECT:
            m_error = "";
            m_gprs->httpPOST(m_url, m_buffer, m_buflen, "application/cbor");
            m_counter = 0;
            next_state = GPRS_HTTPS_CONNECT_WAIT;
            break;
        case GPRS_HTTPS_CONNECT_WAIT:
            if (m_gprs->httpsIsConnected() != 0) {
                if (m_gprs->httpState() == HTTP_ERROR) {
                    m_error = "Connect error";
                }
                next_state = GPRS_HTTPS_DONE;
            } else {
                m_counter++;
                if (!m_gprs->available()) {
                    next_state = GPRS_RESET;
                } else if (m_counter > 25) {
                    m_error = "Timeout.";
                    next_state = GPRS_HTTPS_DONE;
                }
                m_gprs->purgeSerial();
            }
            break;
        case GPRS_HTTPS_DONE:
            m_buflen = 0;
            if (!isDisabled()) {
                next_state = GPRS_HTTPS_READY;
            } else {
                next_state = GPRS_DISABLED;
            }
            break;
        case GPRS_RESET:
            m_buflen = 0;
            next_state = GPRS_DISABLED;
            break;
        default:
            break;
    }

    m_state = next_state;
}

void GPRS::setApn(uint8_t *apn)
{
    strncpy(m_apn, apn, MAX_APN_LEN);
}

void GPRS::setUrl(uint8_t *url)
{
    strncpy(m_url, url, MAX_URL_LEN);
}

int8_t GPRS::getRssi(void)
{
    if (isDisabled()) {
        return -128;

    return m_gprs->getSignalQuality();
}

uint8_t *GPRS::getNetworkName(void)
{
    m_gprs->getOperatorName();
    return m_gprs->buffer();
}

GSM_LOCATION *GPRS::getLocation(void)
{
    m_gprs->getLocation(&m_location);

    uint32_t m = m_location.month;
    uint32_t y = m_location.year + 2000 - (m <= 2 ? 1 : 0);
    uint32_t era = y / 400;
    uint32_t yoe = y - (era * 400);
    uint32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + m_location.day - 1;
    uint32_t doe = yoe * 365 + yoe / 4 - yoe / 10 + doy;
    uint32_t civil = era * 146097 + doe - 719468;

    m_epochTime = ((civil * 24 + m_location.hour) * 60 + m_location.minute) *
                  60 + m_location.second;

    return &m_location;
}

bool GPRS::sendCborPacket(uint8_t source, uint8_t *payload, uint8_t len)
{
    getLocation();    

    uint32_t timeDiff = m_epochTime - m_lastEpochTime;
    bool sendLocation = (timeDiff >= 3600);

    CborMessageInitialize();
    CborMessageAddMap(sendLocation ? 4 : 3);
    if (sendLocation) {
        CborMapAddLocation(m_location.lat, m_location.lon);
        m_lastEpochTime = m_epochTime;
    }
    CborMapAddTimestamp(m_location.year + 2000, m_location..month,
                        m_location.day, m_location.hour,
                        m_location.minute, m_location.second);
    CborMapAddInteger(CBOR_KEY_SOURCE, source);
    CborMapAddCborPayload(payload, len);

    uint8_t *buffer = &m_nextBuffer[m_nextBuflen];
    uint8_t len = MAX_BUFFER_LEN - m_nextBuflen;

    bool buffFit = CborMessageBuffer(&buffer, &len);
    if (buffFit) {
        m_nextBuflen += len;
    }

    if (m_state == GPRS_HTTPS_READY && m_buflen == 0) {
        send_buffer();

        if (!buffFit) {
            CborMessageBuffer(&buffer, &len);
            m_nextBuflen += len;
        }
    }
}

void GPRS::send_buffer(void)
{
    memcpy(m_buffer, m_nextBuffer, m_nextBuflen);
    m_buflen = m_nextBuflen;
    m_nextBuflen = 0;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
