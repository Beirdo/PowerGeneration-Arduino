#include <inttypes.h>
#include "utils.h"

uint8_t atou8(uint8_t *str)
{
    uint8_t value = 0;
    bool valid;
    uint8_t ch;

    do {
        ch = *str;
        valid = true;
        if (ch >= '0' && ch <= '9') {
            ch -= '0';
        } else if (ch >= 'a' && ch <= 'f') {
            ch -= ('a' - 10);
        } else if (ch >= 'A' && ch <= 'F') {
            ch -= ('A' - 10);
        } else {
            valid = false;
        }

        if (valid) {
            value <<= 4;
            value += ch;
        }
    } while(valid);

    return value;
}


// vim:ts=4:sw=4:ai:et:si:sts=4
