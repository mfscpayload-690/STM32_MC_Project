/**
 * @file oled_ssd1306.cpp
 * @brief SSD1306 OLED implementation using verified Adafruit_SSD1306 driver
 */

#include "oled_ssd1306.h"
#include <string.h>

#if defined(ARDUINO)
#ifndef GCC_VERSION
#define GCC_VERSION 140201
#endif
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 s_display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
#endif

SSD1306Display OLED;

SSD1306Display::SSD1306Display()
    : _addr(OLED_I2C_ADDR), _cursor_x(0), _cursor_y(0), _text_color(1), _text_size(1) {
    memset(_buffer, 0, sizeof(_buffer));
}

bool SSD1306Display::begin(uint8_t i2c_addr) {
    _addr = i2c_addr;
#if defined(ARDUINO)
    Wire.begin();
    if (!s_display.begin(SSD1306_SWITCHCAPVCC, _addr)) {
        return false;
    }
    s_display.clearDisplay();
    s_display.setTextColor(SSD1306_WHITE);
    s_display.setTextSize(1);
    s_display.display();
    return true;
#else
    clear();
    return true;
#endif
}

void SSD1306Display::clear() {
#if defined(ARDUINO)
    s_display.clearDisplay();
#else
    memset(_buffer, 0, sizeof(_buffer));
#endif
    _cursor_x = 0;
    _cursor_y = 0;
}

void SSD1306Display::display() {
#if defined(ARDUINO)
    s_display.display();
#endif
}

void SSD1306Display::setPixel(int16_t x, int16_t y, uint8_t color) {
#if defined(ARDUINO)
    s_display.drawPixel(x, y, color ? SSD1306_WHITE : SSD1306_BLACK);
#else
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    if (color) {
        _buffer[x + (y / 8) * OLED_WIDTH] |= (1 << (y % 8));
    } else {
        _buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y % 8));
    }
#endif
}

void SSD1306Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
#if defined(ARDUINO)
    s_display.drawLine(x0, y0, x1, y1, color ? SSD1306_WHITE : SSD1306_BLACK);
#endif
}

void SSD1306Display::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
#if defined(ARDUINO)
    s_display.drawRect(x, y, w, h, color ? SSD1306_WHITE : SSD1306_BLACK);
#endif
}

void SSD1306Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
#if defined(ARDUINO)
    s_display.fillRect(x, y, w, h, color ? SSD1306_WHITE : SSD1306_BLACK);
#endif
}

void SSD1306Display::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t progress_percent) {
    if (progress_percent > 100) progress_percent = 100;
#if defined(ARDUINO)
    s_display.drawRect(x, y, w, h, SSD1306_WHITE);
    int16_t fill_w = (int16_t)((w - 4) * progress_percent / 100);
    if (fill_w > 0) {
        s_display.fillRect(x + 2, y + 2, fill_w, h - 4, SSD1306_WHITE);
    }
#endif
}

void SSD1306Display::setCursor(int16_t x, int16_t y) {
    _cursor_x = x;
    _cursor_y = y;
#if defined(ARDUINO)
    s_display.setCursor(x, y);
#endif
}

void SSD1306Display::setTextColor(uint8_t color) {
    _text_color = color;
#if defined(ARDUINO)
    if (color == 0) {
        // Highlighted / Inverted text (black on white)
        s_display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        // Normal text (white on black)
        s_display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
#endif
}

void SSD1306Display::setTextSize(uint8_t size) {
    _text_size = (size > 0) ? size : 1;
#if defined(ARDUINO)
    s_display.setTextSize(_text_size);
#endif
}

void SSD1306Display::print(char c) {
#if defined(ARDUINO)
    s_display.write(c);
#endif
}

void SSD1306Display::print(const char *text) {
    if (!text) return;
#if defined(ARDUINO)
    s_display.print(text);
#endif
}

void SSD1306Display::print(int value) {
#if defined(ARDUINO)
    s_display.print(value);
#endif
}
