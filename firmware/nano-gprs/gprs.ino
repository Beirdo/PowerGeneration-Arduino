#include <avr/pgmspace.h>
#include <string.h>
#include <SIM800.h>
#include "cbormap.h"

// Ting's default
static const PROGMEM char default_apn[] = "wholesale";
static const PROGMEM char default_url[] = "https://apigatewayurl/method";
static const PROGMEM char default_mime[] = "application/cbor";

GPRS::GPRS(int8_t reset_pin, int8_t enable_pin, int8_t dtr_pin)
{
    m_gprs = new CGPRS_SIM800(&Serial, reset_pin, enable_pin, dtr_pin);
    m_reset_pin = reset_pin;
    m_enable_pin = enable_pin;
    m_dtr_pin = dtr_pin;
    m_state = GPRS_DISABLED;
    memset(&m_location, 0x00, 48);
    m_packetCount = 0;
    m_fram = NULL;
    m_url_cache = NULL;
    m_apn_cache = NULL;
    m_tx_cache = NULL;
    m_tx2_cache = NULL;
}

void GPRS::attachRAM(Adafruit_FRAM_SPI *fram)
{
    m_fram = fram;
    m_gprs->attachRAM(m_fram);
    m_url_cache = new Cache_Segment(m_fram, 0x0C00, 128, 32, 32, NULL, false);
    m_apn_cache = new Cache_Segment(m_fram, 0x0C80, 64, 32, 32,
                                    m_url_cache->buffer(), false);
    m_mime_cache = new Cache_Segment(m_fram, 0x0CC0, 64, 32, 32,
                                    m_url_cache->buffer(), false);
    m_tx_cache = new Cache_Segment(m_fram, 0x0D00, 256, 32, 32,
                                   m_url_cache->buffer(), true);
    m_tx2_cache = new Cache_Segment(m_fram, 0x0E00, 256, 32, 32,
                                    m_url_cache->buffer(), true);

    setUrl(default_url, true);
    setAPN(default_apn, true);
    setMime(default_mime, true);
    m_tx_cache->clear();
    m_tx2_cache->clear();
}

void GPRS::setUrl(const char *url, bool is_pgmspace)
{
    char ch;
    uint16_t i;
    uint16_t len;

    if (is_pgmspace) {
        len = strlen_P(url);
    } else {
        len = strlen(url);
    }

    m_url_cache->setWriteProtect(false);
    m_url_cache->clear();
    for (i = 0; i < len; i++) {
        if (is_pgmspace) {
            ch = pgm_read_byte(url + i);
        } else {
            ch = url[i];
        }
        m_url_cache->write(i, ch);
    }
    m_url_cache->write(i, '\0');
    m_url_cache->flushCacheLine();
    m_url_cache->setWriteProtect(true);
}

void GPRS::setAPN(const char *apn, bool is_pgmspace)
{
    char ch;
    uint16_t i;
    uint16_t len;

    if (is_pgmspace) {
        len = strlen_P(apn);
    } else {
        len = strlen(apn);
    }

    m_apn_cache->setWriteProtect(false);
    m_apn_cache->clear();
    for (i = 0; i < len; i++) {
        if (is_pgmspace) {
            ch = pgm_read_byte(apn + i);
        } else {
            ch = apn[i];
        }
        m_apn_cache->write(i, ch);
    }
    m_apn_cache->write(i, '\0');
    m_apn_cache->flushCacheLine();
    m_apn_cache->setWriteProtect(true);
}

void GPRS::setMime(const char *mime, bool is_pgmspace)
{
    char ch;
    uint16_t i;
    uint16_t len;

    if (is_pgmspace) {
        len = strlen_P(mime);
    } else {
        len = strlen(mime);
    }

    m_mime_cache->setWriteProtect(false);
    m_mime_cache->clear();
    for (i = 0; i < len; i++) {
        if (is_pgmspace) {
            ch = pgm_read_byte(mime + i);
        } else {
            ch = mime[i];
        }
        m_mime_cache->write(i, ch);
    }
    m_mime_cache->write(i, '\0');
    m_mime_cache->flushCacheLine();
    m_mime_cache->setWriteProtect(true);
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
            if (m_gprs->setup(m_apn_cache) == 0) {
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
                m_gprs->httpUninit();
                delay(1000);
            }
            break;
        case GPRS_HTTPS_READY:
            if (m_tx_cache->circularReadAvailable()) {
                next_state = GPRS_HTTPS_CONNECT;
            }
            break;
        case GPRS_HTTPS_CONNECT:
            m_error[0] = '\0';
            m_gprs->httpPOST(m_url_cache, m_tx_cache, m_mime_cache);
            m_counter = 0;
            next_state = GPRS_HTTPS_CONNECT_WAIT;
            break;
        case GPRS_HTTPS_CONNECT_WAIT:
            if (m_gprs->httpIsConnected() != 0) {
                if (m_gprs->httpState() == HTTP_ERROR) {
                    strcpy(m_error, "Connect error");
                }
                next_state = GPRS_HTTPS_DONE;
            } else {
                m_counter++;
                if (!m_gprs->available()) {
                    next_state = GPRS_RESET;
                } else if (m_counter > 25) {
                    strcpy(m_error, "Timeout");
                    next_state = GPRS_HTTPS_DONE;
                }
                m_gprs->purgeSerial();
            }
            break;
        case GPRS_HTTPS_DONE:
            m_tx_cache->clear();
            if (!isDisabled()) {
                next_state = GPRS_HTTPS_READY;
                if (m_tx2_cache->circularWriteAvailable() < MAX_BUFFER_LEN) {
                    swap_buffers();
                }
            } else {
                next_state = GPRS_DISABLED;
            }
            break;
        case GPRS_RESET:
            m_tx_cache->clear();
            next_state = GPRS_DISABLED;
            break;
        default:
            break;
    }

    m_state = next_state;
}

int8_t GPRS::getRssi(void)
{
    if (isDisabled()) {
        return -128;
    }

    return m_gprs->getSignalQuality();
}

uint8_t *GPRS::getNetworkName(void)
{
    m_gprs->getOperatorName();
    return m_gprs->buffer();
}

uint8_t *GPRS::getLocation(void)
{
    m_gprs->getLocation((char *)m_location, 48);

    return m_location;
}

bool GPRS::sendCborPacket(uint8_t source, uint8_t *payload, uint8_t len)
{
    getLocation();
    // m_location has: -73.993149,40.729370,2015/03/07,19:01:08

    bool sendLocation = (m_packetCount++ & 127) == 0;
    char *p = (char *)m_location;

    CborMessageInitialize();
    CborMessageAddMap(sendLocation ? 4 : 3);
    if (sendLocation) {
        float lat = (float)atoi(p);
        while (*p && p != '.') {
            p++;
        }
        lat += ((float)atoi(++p) * 1e-6);

        while (*p && p != ',') {
            p++;
        }

        float lon = (float)atoi(++p);
        while (*p && p != '.') {
            p++;
        }
        lon += ((float)atoi(++p) * 1e-6);

        while (*p && p != ',') {
            p++;
        }
        p++;

        CborMapAddLocation(lat, lon);
    } else {
        // skip lat/lon
        while (*p && p != ',') {
            p++;
        }
        p++;

        while (*p && p != ',') {
            p++;
        }
        p++;
    }
    CborMapAddTimestamp((char *)p);
    CborMapAddInteger(CBOR_KEY_SOURCE, source);
    CborMapAddCborPayload(payload, len);

    uint8_t *buffer = m_error;
    len = min(m_tx2_cache->circularWriteAvailable(), MAX_BUFFER_LEN);

    bool buffFit = CborMessageBuffer(&buffer, &len);
    if (buffFit) {
        m_tx2_cache->circularWrite(buffer, len);
        if (m_tx2_cache->circularWriteAvailable() < MAX_BUFFER_LEN) {
            swap_buffers();
        }
        return;
    }

    if (m_state == GPRS_HTTPS_READY && !m_tx_cache->circularReadAvailable()) {
        swap_buffers();

        if (!buffFit) {
            CborMessageBuffer(&buffer, &len);
            m_tx2_cache->circularWrite(buffer, len);
        }
    }
}

void GPRS::swap_buffers(void)
{
    if (m_tx_cache->circularReadAvailable()) {
        return;
    }
    Cache_Segment *temp = m_tx_cache;
    m_tx_cache = m_tx2_cache;
    m_tx2_cache = temp;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
