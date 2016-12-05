#include <SPI.h>
#include "SdFat.h"

#define FILE_BASE_NAME "cbor"

#define error(msg) sd.errorHalt(F(msg))

// File system object.
SdFat sd;

// Log file.
SdFile file;

void SDCardInitialize(uint8_t cs)
{
    const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
    char fileName[13] = FILE_BASE_NAME "0000.log";

    // Initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
    // breadboards.  use SPI_FULL_SPEED for better performance.
    if (!sd.begin(cs, SPI_FULL_SPEED)) {
        sd.initErrorHalt();
    }

    // Find an unused file name.
    if (BASE_NAME_SIZE > 6) {
        error("FILE_BASE_NAME too long");
    }
    while (sd.exists(fileName)) {
        if (fileName[BASE_NAME_SIZE + 3] != '9') {
            fileName[BASE_NAME_SIZE + 3]++;
        } else if (filename[BASE_NAME_SIZE + 2] != '9') {
            fileName[BASE_NAME_SIZE + 3] = '0';
            fileName[BASE_NAME_SIZE + 2]++;
        } else if (filename[BASE_NAME_SIZE + 1] != '9') {
            fileName[BASE_NAME_SIZE + 3] = '0';
            fileName[BASE_NAME_SIZE + 2] = '0';
            fileName[BASE_NAME_SIZE + 1]++;
        } else if (fileName[BASE_NAME_SIZE] != '9') {
            fileName[BASE_NAME_SIZE + 3] = '0';
            fileName[BASE_NAME_SIZE + 2] = '0';
            fileName[BASE_NAME_SIZE + 1] = '0';
            fileName[BASE_NAME_SIZE]++;
        } else {
            error("Can't create file name");
        }
    }

    if (!file.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
        error("file.open");
    }
}

void SDCardWrite(uint8_t *buffer, uint8_t len)
{
    file.write(buffer, len);
}

// vim:ts=4:sw=4:ai:et:si:sts=4
