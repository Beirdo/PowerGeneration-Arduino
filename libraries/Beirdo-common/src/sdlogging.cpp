#include <SPI.h>
#include <SdFat.h>
#include "sdlogging.h"

#define error(msg) m_sd.errorHalt(F(msg))

SDLogging::SDLogging(uint8_t cs) : m_sd(SdFat()), m_file(SdFile())
{
    m_baseNameSize = sizeof(FILE_BASE_NAME) - 1;
    m_filename = FILE_BASE_NAME "0000.log";

    // Initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
    // breadboards.  use SPI_FULL_SPEED for better performance.
    if (!m_sd.begin(cs, SPI_FULL_SPEED)) {
        m_sd.initErrorHalt();
    }

    // Find an unused file name.
    if (m_baseNameSize > 6) {
        error("FILE_BASE_NAME too long");
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
        }
    }

    if (!m_file.open(m_filename, O_CREAT | O_WRITE | O_EXCL)) {
        error("file.open");
    }
}

void SDLogging::write(uint8_t *buffer, uint8_t len)
{
    m_file.write(buffer, len);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
