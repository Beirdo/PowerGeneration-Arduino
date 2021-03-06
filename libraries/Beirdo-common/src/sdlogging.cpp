#include <SPI.h>
#include <SdFat.h>
#include <string.h>
#include "sdlogging.h"

#define error(msg) m_sd.errorHalt(F(msg))

SDLogging::SDLogging(uint8_t cs, uint8_t cd) : m_sd(SdFat()), m_file(SdFile())
{
    m_initialized = false;
    m_cd = cd;
    pinMode(cd, INPUT);

    if (digitalRead(m_cd)) {
        error("No card inserted");
        return;
    }

    m_baseNameSize = sizeof(FILE_BASE_NAME) - 1;
    strcpy(m_filename, FILE_BASE_NAME);
    strcat(m_filename, "0000.log");

    // Initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
    // breadboards.  use SPI_FULL_SPEED for better performance.
    if (!m_sd.begin(cs, SPI_FULL_SPEED)) {
        m_sd.initErrorHalt();
        return;
    }

    // Find an unused file name.
    if (m_baseNameSize > 6) {
        error("FILE_BASE_NAME too long");
        return;
    }

    while (m_sd.exists(m_filename)) {
        if (m_filename[m_baseNameSize + 3] != '9') {
            m_filename[m_baseNameSize + 3]++;
        } else if (m_filename[m_baseNameSize + 2] != '9') {
            m_filename[m_baseNameSize + 3] = '0';
            m_filename[m_baseNameSize + 2]++;
        } else if (m_filename[m_baseNameSize + 1] != '9') {
            m_filename[m_baseNameSize + 3] = '0';
            m_filename[m_baseNameSize + 2] = '0';
            m_filename[m_baseNameSize + 1]++;
        } else if (m_filename[m_baseNameSize] != '9') {
            m_filename[m_baseNameSize + 3] = '0';
            m_filename[m_baseNameSize + 2] = '0';
            m_filename[m_baseNameSize + 1] = '0';
            m_filename[m_baseNameSize]++;
        } else {
            error("Can't create file name");
            return;
        }
    }

    if (!m_file.open(m_filename, O_CREAT | O_WRITE | O_EXCL)) {
        error("file.open");
        return;
    }

    m_initialized = true;
}

void SDLogging::write(uint8_t *buffer, uint8_t len)
{
    if (m_initialized) {
        m_file.write(buffer, len);
    }
}

// vim:ts=4:sw=4:ai:et:si:sts=4
