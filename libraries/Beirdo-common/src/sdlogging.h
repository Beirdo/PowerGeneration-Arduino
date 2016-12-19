#ifndef SDLOGGING_H__
#define SDLOGGING_H__

#include <inttypes.h>
#include <SdFat.h>

#ifndef FILE_BASE_NAME
#define FILE_BASE_NAME "cbor"
#endif

class SDLogging {
    public:
        SDLogging(uint8_t cs);
        void write(uint8_t *buffer, uint8_t len);
    protected:
        SdFat m_sd;
        SdFile m_file;
        uint8_t m_baseNameSize;
        char m_filename[13];
};

#endif

// vim:ts=4:sw=4:ai:et:si:sts=4
