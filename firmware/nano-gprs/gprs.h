#ifndef GPRS_H__
#define GPRS_H__

#include <Adafruit_FRAM_SPI.h>
#include <SIM800.h>
#include <inttypes.h>

#define MAX_APN_LEN 64
#define MAX_URL_LEN 128
#define MAX_ERROR_LEN 32
#define MAX_BUFFER_LEN 32

typedef enum {
    GPRS_DISABLED,
    GPRS_INIT,
    GPRS_SETUP,
    GPRS_READY,
    GPRS_HTTPS_INIT,
    GPRS_HTTPS_READY,
    GPRS_HTTPS_CONNECT,
    GPRS_HTTPS_CONNECT_WAIT,
    GPRS_HTTPS_DONE,
    GPRS_RESET,
} gprs_state_t;

class GPRS {
    public:
        GPRS(int8_t reset_pin, int8_t enable_pin, int8_t dtr_pin);
        void stateMachine(void);
        void attachRAM(Adafruit_FRAM_SPI *fram); 

        bool isDisabled(void);
        int8_t getRssi(void);
        uint8_t *getNetworkName(void);
        GSM_LOCATION *getLocation(void);
        bool sendCborPacket(uint8_t source, uint8_t *payload, uint8_t len);
        gprs_state_t getState(void) { return m_state; };
        uint8_t *getError(void) { return m_error; };

        void setUrl(const char *url, bool is_pgmspace);
        void setAPN(const char *apn, bool is_pgmspace);
        void setMime(const char *mime, bool is_pgmspace);

    protected:
        void swap_buffers(void);

        int8_t m_reset_pin;
        int8_t m_enable_pin;
        int8_t m_dtr_pin;
        CGPRS_SIM800 *m_gprs;
        GSM_LOCATION m_location;
        uint32_t m_epochTime;
        uint32_t m_lastEpochTime;
        gprs_state_t m_state;

        Adafruit_FRAM_SPI *m_fram;
        Cache_Segment *m_apn_cache;
        Cache_Segment *m_url_cache;
        Cache_Segment *m_mime_cache;
        volatile Cache_Segment *m_tx_cache;
        volatile Cache_Segment *m_tx2_cache;

        uint8_t m_error[MAX_ERROR_LEN];
        uint8_t m_counter;
};

#endif

// vim:ts=4:sw=4:ai:et:si:sts=4
