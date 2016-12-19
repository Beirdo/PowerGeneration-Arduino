#ifndef LCDSCREEN_H__
#define LCDSCREEN_H__

#include <Adafruit_GFX.h>
#include "linkedlist.h"

typedef void (*formatter_t)(void *, uint8_t *, uint8_t, uint8_t *);

void formatAutoScale(void *valptr, uint8_t *buffer, uint8_t maxlen,
                     uint8_t *units);
void formatTemperature(void *valptr, uint8_t *buffer, uint8_t maxlen,
                       uint8_t *units);


class LCDScreen {
    public:
        LCDScreen(uint8_t *title, void *variable, formatter_t *formatter, 
                  uint8_t *units);
        void title_line(uint8_t **buffer, uint8_t maxlen);
        void data_line(uint8_t **buffer, uint8_t maxlen);
    protected:
        uint8_t *m_title;
        void *m_variable;
        uint8_t *m_units;
        formatter_t *m_formatter;
};

class LCDDeck {
    public:
        LCDDeck(Adafruit_GFX *display);
        void resetIndex(uint8_t index);
        void formatFrame(uint8_t index);
        void displayFrame(void);
        int8_t nextIndex(void);
        void addFrame(LCDScreen *frame);
    protected:
        LCDScreen *getFrame(uint8_t index);
        void displayString(int16_t &y, int16_t &h, uint8_t *str);
        void generateIndicator(void);

        LinkedList m_frameList;
        Adafruit_GFX *m_display;
        int16_t m_height;
        int16_t m_width;

        uint8_t m_frameCount;
        int8_t m_index;
        LCDScreen *m_currentFrame;
        uint8_t m_data_buffer[64];
        uint8_t m_title_blank[1];
        uint8_t *m_title;
        uint8_t m_indicator[64];
};


#endif

// vim:ts=4:sw=4:ai:et:si:sts=4


