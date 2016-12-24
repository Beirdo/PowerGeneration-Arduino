#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "lcdscreen.h"
#include "linkedlist.h"

#define digit(x)  ((char)(((x) + 0x30) & 0xFF))
#define split_value(value, part, scale)  \
  do { \
    part = value % scale; \
    value = value / scale; \
  } while(0)

static uint8_t digitCount(int32_t value, bool &negative);

static uint8_t digitCount(int32_t value, bool &negative)
{
    int32_t tempvalue = value;
    if (tempvalue < 0) {
        negative = true;
        tempvalue = -tempvalue;
    } else {
        negative = false;
        if (tempvalue == 0) {
            return (uint8_t)1;
        }
    }

    uint8_t digitcount = 0;
    for (; tempvalue; tempvalue /= 10) {
        digitcount++;
    }

    return digitcount;
}

void formatAutoScale(void *valptr, uint8_t *buffer, uint8_t maxlen,
                     uint8_t *units)
{
    int32_t value = *(int32_t *)valptr;

    if (value <= 0) {
        buffer[0] = '0';
        strcpy(&buffer[1], units);
        return;
    }

    bool negative;
    uint8_t digitcount = digitCount(value, negative);

    char subUnit = digitcount / 3;

    switch (subUnit) {
        case 0:
            subUnit = 'm';
            break;
        case 1:
            subUnit = '\0';
            break;
        case 2:
            subUnit = 'k';
            break;
    }

    char precision = 4;
    char digits = digitcount % 3;
    digits = (digits ? digits : 3);

    char decimals = (precision > digits && subUnit != 'm' ?
                     precision - digits : 0);
    digits += decimals;

    long scale;
    for (scale = 1; digitcount > digits; scale *= 10) {
        digitcount--;
    }

    value /= scale;

    int8_t index = digits + (decimals ? 1 : 0) - 1;
    uint8_t unitIndex = index + 1;
    if (subUnit) {
        buffer[unitIndex++] = subUnit;
    }

    if (units) {
        strcpy(&buffer[unitIndex], units);
    } else {
        buffer[unitIndex] = '\0';
    }

    char newdigit;
    for (; digits > 0; digits--) {
        split_value(value, newdigit, 10);
        buffer[index--] = digit(newdigit);
        if (decimals) {
            decimals--;
            if (!decimals) {
                buffer[index--] = '.';
            }
        }
    }
}

void formatTemperature(void *valptr, uint8_t *buffer, uint8_t maxlen,
                       uint8_t *units)
{
    int16_t value = *(int16_t *)valptr;
    bool negative;
    uint8_t digitcount = digitCount(value, negative);
    if (digitcount < 2) {
        digitcount = 2;
    }
    int8_t index = digitcount + 1 + (negative ? 1 : 0) - 1;
    uint8_t unitIndex = index + 1;

    if (units) {
        strcpy(&buffer[unitIndex], units);
    } else {
        buffer[unitIndex] = '\0';
    }

    value = (negative ? -value : value);

    char part;
    split_value(value, part, 10);
    buffer[index--] = digit(part);
    buffer[index--] = '.';

    for (char i = 0; index >= 0 && (value != 0 || index >= (char)negative);
         i++) {
        split_value(value, part, 10);
        buffer[index--] = digit(part);
    }

    if (negative) {
        buffer[index] = '-';
    }
}

LCDScreen::LCDScreen(char *title, void *variable, formatter_t formatter, 
                     char *units)
{
    m_title = (uint8_t *)title;
    m_variable = variable;
    m_formatter = formatter;
    m_units = (uint8_t *)units;
}

void LCDScreen::title_line(uint8_t **buffer, uint8_t maxlen)
{
    if (m_title) {
        *buffer = m_title;
    } else {
        **buffer = '\0';
    }
}

void LCDScreen::data_line(uint8_t **buffer, uint8_t maxlen)
{
    if (!m_formatter || !m_variable) {
        **buffer = '\0';
        return;
    }

    m_formatter(m_variable, *buffer, maxlen, m_units);
}

LCDDeck::LCDDeck(Adafruit_GFX *display, bool is_ssd1306) :
        m_frameList(LinkedList())
{
    m_display = display;
    m_is_ssd1306 = is_ssd1306;
    m_frameCount = 0;
    m_index = -1;
    m_height = display->height();
    m_width = display->width();
    m_title_blank[0] = '\0';
}

void LCDDeck::resetIndex(uint8_t index)
{
    m_index = (int8_t)index;
    formatFrame(index);
}

void LCDDeck::formatFrame(uint8_t index)
{
    if (!getFrame(index)) {
        return;
    }

    m_title = &m_title_blank[0];
    m_currentFrame->title_line(&m_title, (uint8_t)64);
    uint8_t *buffer = &m_data_buffer[0];
    m_currentFrame->data_line(&buffer, (uint8_t)64);
}

void LCDDeck::displayString(int16_t &y, int16_t &h, uint8_t *str)
{
    int16_t x = 0, w = 0;

    // Put the string, horizontally centered.
    m_display->getTextBounds((char *)str, 0, y, &x, &y, &w, &h);
    x += ((m_width - w) / 2);
    m_display->setCursor(x, y);

    for (uint8_t *ch = str; *ch; ch++) {
        m_display->write(*ch);
    }
}

void LCDDeck::displayIndicator(void)
{
    int16_t x, y, w, h;

    m_display->setTextSize(1);
    y = m_height - 8;

    m_display->getTextBounds((char *)"o", 0, y, &x, &y, &w, &h);
    x += ((m_width - (w * (2 * m_frameCount - 1))) / 2);

    m_display->setCursor(x, y);

    for (uint8_t i = 0; i < m_frameCount; i++) {
        m_display->write(i == m_index ? 'o' : '.');
        m_display->write(' ');
    }
}

void LCDDeck::displayFrame(void)
{
    if (m_is_ssd1306) {
        m_display->clearDisplay();
    } else {
        m_display->fillScreen(BLACK);
    }

    m_display->setTextColor(WHITE);
    m_display->setTextSize(2);

    // Put the title, centered.  Leave 8 pixels above for RSSI, etc
    int16_t y = 16;
    int16_t h;

    displayString(y, h, m_title);

    // Leave 4 pixels extra, put value, centered
    m_display->setTextSize(2);
    y += h + 4;
    displayString(y, h, m_data_buffer);

    // Put screen index indicator at bottom line, centered
    displayIndicator();

    if (m_is_ssd1306) {
        m_display->display();
    }
}

int8_t LCDDeck::nextIndex(void)
{
    return (m_index + 1) % m_frameCount;
}

void LCDDeck::addFrame(LCDScreen *frame)
{
    m_frameList.add((void *)frame);
    m_frameCount++;
}

LCDScreen *LCDDeck::getFrame(uint8_t index)
{
    LCDScreen *frame;
    uint8_t i;

    for (i = 0, frame = m_frameList.head(); frame && i < index; i++) {
        frame = m_frameList.next();
    }

    if (frame) {
        m_currentFrame = frame;
        m_index = index;
    }
    return frame;
}

// vim:ts=4:sw=4:ai:et:si:sts=4
