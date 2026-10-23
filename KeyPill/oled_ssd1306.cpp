/**
 * @file oled_ssd1306.cpp
 * @brief Self-contained 128x64 I2C SSD1306 OLED Implementation
 */

#include "oled_ssd1306.h"
#include <string.h>

#if defined(ARDUINO)
#include <Arduino.h>
#include <Wire.h>
#endif

// Standard 5x7 ASCII font table (offset 32 ' ' to 126 '~')
static const uint8_t font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // 32 ' '
    0x00, 0x00, 0x5F, 0x00, 0x00, // 33 '!'
    0x00, 0x07, 0x00, 0x07, 0x00, // 34 '"'
    0x14, 0x7F, 0x14, 0x7F, 0x14, // 35 '#'
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // 36 '$'
    0x23, 0x13, 0x08, 0x64, 0x62, // 37 '%'
    0x36, 0x49, 0x55, 0x22, 0x50, // 38 '&'
    0x00, 0x05, 0x03, 0x00, 0x00, // 39 '''
    0x00, 0x1C, 0x22, 0x41, 0x00, // 40 '('
    0x00, 0x41, 0x22, 0x1C, 0x00, // 41 ')'
    0x14, 0x08, 0x3E, 0x08, 0x14, // 42 '*'
    0x08, 0x08, 0x3E, 0x08, 0x08, // 43 '+'
    0x00, 0x50, 0x30, 0x00, 0x00, // 44 ','
    0x08, 0x08, 0x08, 0x08, 0x08, // 45 '-'
    0x00, 0x60, 0x60, 0x00, 0x00, // 46 '.'
    0x20, 0x10, 0x08, 0x04, 0x02, // 47 '/'
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 48 '0'
    0x00, 0x42, 0x7F, 0x40, 0x00, // 49 '1'
    0x42, 0x61, 0x51, 0x49, 0x46, // 50 '2'
    0x21, 0x41, 0x45, 0x4B, 0x31, // 51 '3'
    0x18, 0x14, 0x12, 0x7F, 0x10, // 52 '4'
    0x27, 0x45, 0x45, 0x45, 0x39, // 53 '5'
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 54 '6'
    0x01, 0x71, 0x09, 0x05, 0x03, // 55 '7'
    0x36, 0x49, 0x49, 0x49, 0x36, // 56 '8'
    0x06, 0x49, 0x49, 0x29, 0x1E, // 57 '9'
    0x00, 0x36, 0x36, 0x00, 0x00, // 58 ':'
    0x00, 0x56, 0x36, 0x00, 0x00, // 59 ';'
    0x08, 0x14, 0x22, 0x41, 0x00, // 60 '<'
    0x14, 0x14, 0x14, 0x14, 0x14, // 61 '='
    0x00, 0x41, 0x22, 0x14, 0x08, // 62 '>'
    0x02, 0x01, 0x51, 0x09, 0x06, // 63 '?'
    0x32, 0x49, 0x79, 0x41, 0x3E, // 64 '@'
    0x7E, 0x11, 0x11, 0x11, 0x7E, // 65 'A'
    0x7F, 0x49, 0x49, 0x49, 0x36, // 66 'B'
    0x3E, 0x41, 0x41, 0x41, 0x22, // 67 'C'
    0x7F, 0x41, 0x41, 0x22, 0x1C, // 68 'D'
    0x7F, 0x49, 0x49, 0x49, 0x41, // 69 'E'
    0x7F, 0x09, 0x09, 0x09, 0x01, // 70 'F'
    0x3E, 0x41, 0x49, 0x49, 0x7A, // 71 'G'
    0x7F, 0x08, 0x08, 0x08, 0x7F, // 72 'H'
    0x00, 0x41, 0x7F, 0x41, 0x00, // 73 'I'
    0x20, 0x40, 0x41, 0x3F, 0x01, // 74 'J'
    0x7F, 0x08, 0x14, 0x22, 0x41, // 75 'K'
    0x7F, 0x40, 0x40, 0x40, 0x40, // 76 'L'
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // 77 'M'
    0x7F, 0x04, 0x08, 0x10, 0x7F, // 78 'N'
    0x3E, 0x41, 0x41, 0x41, 0x3E, // 79 'O'
    0x7F, 0x09, 0x09, 0x09, 0x06, // 80 'P'
    0x3E, 0x41, 0x51, 0x21, 0x5E, // 81 'Q'
    0x7F, 0x09, 0x19, 0x29, 0x46, // 82 'R'
    0x46, 0x49, 0x49, 0x49, 0x31, // 83 'S'
    0x01, 0x01, 0x7F, 0x01, 0x01, // 84 'T'
    0x3F, 0x40, 0x40, 0x40, 0x3F, // 85 'U'
    0x1F, 0x20, 0x40, 0x20, 0x1F, // 86 'V'
    0x7F, 0x20, 0x18, 0x20, 0x7F, // 87 'W'
    0x63, 0x14, 0x08, 0x14, 0x63, // 88 'X'
    0x07, 0x08, 0x70, 0x08, 0x07, // 89 'Y'
    0x61, 0x51, 0x49, 0x45, 0x43, // 90 'Z'
    0x00, 0x7F, 0x41, 0x41, 0x00, // 91 '['
    0x02, 0x04, 0x08, 0x10, 0x20, // 92 '\'
    0x00, 0x41, 0x41, 0x7F, 0x00, // 93 ']'
    0x04, 0x02, 0x01, 0x02, 0x04, // 94 '^'
    0x40, 0x40, 0x40, 0x40, 0x40, // 95 '_'
    0x00, 0x01, 0x02, 0x04, 0x00, // 96 '`'
    0x20, 0x54, 0x54, 0x54, 0x78, // 97 'a'
    0x7F, 0x48, 0x44, 0x44, 0x38, // 98 'b'
    0x38, 0x44, 0x44, 0x44, 0x20, // 99 'c'
    0x38, 0x44, 0x44, 0x48, 0x7F, // 100 'd'
    0x38, 0x54, 0x54, 0x54, 0x18, // 101 'e'
    0x08, 0x7E, 0x09, 0x01, 0x02, // 102 'f'
    0x08, 0x54, 0x54, 0x54, 0x3C, // 103 'g'
    0x7F, 0x08, 0x04, 0x04, 0x78, // 104 'h'
    0x00, 0x44, 0x7D, 0x40, 0x00, // 105 'i'
    0x20, 0x40, 0x44, 0x3D, 0x00, // 106 'j'
    0x7F, 0x10, 0x28, 0x44, 0x00, // 107 'k'
    0x00, 0x41, 0x7F, 0x40, 0x00, // 108 'l'
    0x7C, 0x04, 0x18, 0x04, 0x78, // 109 'm'
    0x7C, 0x08, 0x04, 0x04, 0x78, // 110 'n'
    0x38, 0x44, 0x44, 0x44, 0x38, // 111 'o'
    0x7C, 0x14, 0x14, 0x14, 0x08, // 112 'p'
    0x08, 0x14, 0x14, 0x18, 0x7C, // 113 'q'
    0x7C, 0x08, 0x04, 0x04, 0x08, // 114 'r'
    0x48, 0x54, 0x54, 0x54, 0x20, // 115 's'
    0x04, 0x3F, 0x44, 0x40, 0x20, // 116 't'
    0x3C, 0x40, 0x40, 0x20, 0x7C, // 117 'u'
    0x1C, 0x20, 0x40, 0x20, 0x1C, // 118 'v'
    0x3C, 0x40, 0x30, 0x40, 0x3C, // 119 'w'
    0x44, 0x28, 0x10, 0x28, 0x44, // 120 'x'
    0x0C, 0x50, 0x50, 0x50, 0x3C, // 121 'y'
    0x44, 0x64, 0x54, 0x4C, 0x44, // 122 'z'
    0x00, 0x08, 0x36, 0x41, 0x00, // 123 '{'
    0x00, 0x00, 0x7F, 0x00, 0x00, // 124 '|'
    0x00, 0x41, 0x36, 0x08, 0x00, // 125 '}'
    0x08, 0x08, 0x2A, 0x1C, 0x08  // 126 '~'
};

SSD1306Display OLED;

SSD1306Display::SSD1306Display()
    : _addr(OLED_I2C_ADDR), _cursor_x(0), _cursor_y(0), _text_color(1), _text_size(1) {
    memset(_buffer, 0, sizeof(_buffer));
}

void SSD1306Display::sendCommand(uint8_t cmd) {
#if defined(ARDUINO)
    Wire.beginTransmission(_addr);
    Wire.write(0x00); // Co = 0, D/C = 0 -> Command
    Wire.write(cmd);
    Wire.endTransmission();
#else
    (void)cmd;
#endif
}

bool SSD1306Display::begin(uint8_t i2c_addr) {
    _addr = i2c_addr;
#if defined(ARDUINO)
    Wire.begin(); // Join I2C bus as master (PB6 = SCL, PB7 = SDA)
    Wire.setClock(400000); // 400kHz Fast I2C

    // Test communication
    Wire.beginTransmission(_addr);
    if (Wire.endTransmission() != 0) {
        return false; // Display not responding on this address
    }

    // SSD1306 Initialization sequence for 128x64
    sendCommand(0xAE); // Display OFF
    sendCommand(0xD5); // Set display clock divide
    sendCommand(0x80); // Suggested ratio
    sendCommand(0xA8); // Set multiplex ratio
    sendCommand(0x3F); // 64 lines (0x3F = 63)
    sendCommand(0xD3); // Display offset
    sendCommand(0x00); // No offset
    sendCommand(0x40); // Start line 0
    sendCommand(0x8D); // Charge pump
    sendCommand(0x14); // Enable charge pump
    sendCommand(0x20); // Memory addressing mode
    sendCommand(0x00); // Horizontal addressing mode
    sendCommand(0xA1); // Segment remap (column 127 is mapped to SEG0)
    sendCommand(0xC8); // COM scan direction (remapped)
    sendCommand(0xDA); // Set COM pins hardware config
    sendCommand(0x12); // Alternative COM pin configuration
    sendCommand(0x81); // Set contrast control
    sendCommand(0xCF);
    sendCommand(0xD9); // Set pre-charge period
    sendCommand(0xF1);
    sendCommand(0xDB); // Set Vcomh deselect level
    sendCommand(0x40);
    sendCommand(0xA4); // Entire display ON, resume to RAM content
    sendCommand(0xA6); // Set normal display (not inverted)
    sendCommand(0xAF); // Display ON
#endif

    clear();
    display();
    return true;
}

void SSD1306Display::clear() {
    memset(_buffer, 0, sizeof(_buffer));
    _cursor_x = 0;
    _cursor_y = 0;
}

void SSD1306Display::setPixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    if (color) {
        _buffer[x + (y / 8) * OLED_WIDTH] |= (1 << (y % 8));
    } else {
        _buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y % 8));
    }
}

void SSD1306Display::display() {
#if defined(ARDUINO)
    // Set column address (0 to 127)
    sendCommand(0x21);
    sendCommand(0);
    sendCommand(OLED_WIDTH - 1);

    // Set page address (0 to 7)
    sendCommand(0x22);
    sendCommand(0);
    sendCommand((OLED_HEIGHT / 8) - 1);

    // Stream 1024 bytes in chunks of 16 bytes over I2C
    for (uint16_t i = 0; i < sizeof(_buffer); i += 16) {
        Wire.beginTransmission(_addr);
        Wire.write(0x40); // Data mode
        for (uint8_t j = 0; j < 16; ++j) {
            Wire.write(_buffer[i + j]);
        }
        Wire.endTransmission();
    }
#endif
}

void SSD1306Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    int16_t dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (true) {
        setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void SSD1306Display::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    drawLine(x, y, x + w - 1, y, color);
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    drawLine(x, y, x, y + h - 1, color);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void SSD1306Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    for (int16_t i = x; i < x + w; ++i) {
        for (int16_t j = y; j < y + h; ++j) {
            setPixel(i, j, color);
        }
    }
}

void SSD1306Display::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t progress_percent) {
    if (progress_percent > 100) progress_percent = 100;
    drawRect(x, y, w, h, 1);
    int16_t fill_w = (int16_t)((w - 4) * progress_percent / 100);
    if (fill_w > 0) {
        fillRect(x + 2, y + 2, fill_w, h - 4, 1);
    }
}

void SSD1306Display::setCursor(int16_t x, int16_t y) {
    _cursor_x = x;
    _cursor_y = y;
}

void SSD1306Display::setTextColor(uint8_t color) {
    _text_color = color;
}

void SSD1306Display::setTextSize(uint8_t size) {
    _text_size = (size > 0) ? size : 1;
}

void SSD1306Display::drawChar(int16_t x, int16_t y, unsigned char c, uint8_t color, uint8_t size) {
    if (c < 32 || c > 126) c = '?';
    uint16_t char_offset = (c - 32) * 5;

    for (int8_t i = 0; i < 5; ++i) {
        uint8_t line = font5x7[char_offset + i];
        for (int8_t j = 0; j < 8; ++j) {
            if (line & 0x1) {
                if (size == 1) {
                    setPixel(x + i, y + j, color);
                } else {
                    fillRect(x + (i * size), y + (j * size), size, size, color);
                }
            } else if (color == 0) {
                // Inverted background
                if (size == 1) {
                    setPixel(x + i, y + j, 1);
                } else {
                    fillRect(x + (i * size), y + (j * size), size, size, 1);
                }
            }
            line >>= 1;
        }
    }
}

void SSD1306Display::print(char c) {
    if (c == '\n') {
        _cursor_x = 0;
        _cursor_y += _text_size * 8;
        return;
    }
    if (c == '\r') return;

    if (_cursor_x + (_text_size * 6) > OLED_WIDTH) {
        _cursor_x = 0;
        _cursor_y += _text_size * 8;
    }

    drawChar(_cursor_x, _cursor_y, (unsigned char)c, _text_color, _text_size);
    _cursor_x += _text_size * 6;
}

void SSD1306Display::print(const char *text) {
    if (!text) return;
    while (*text) {
        print(*text++);
    }
}

void SSD1306Display::print(int value) {
    char buf[12];
    int idx = 0;
    if (value == 0) {
        print('0');
        return;
    }
    if (value < 0) {
        print('-');
        value = -value;
    }
    while (value > 0) {
        buf[idx++] = '0' + (value % 10);
        value /= 10;
    }
    for (int i = idx - 1; i >= 0; --i) {
        print(buf[i]);
    }
}
