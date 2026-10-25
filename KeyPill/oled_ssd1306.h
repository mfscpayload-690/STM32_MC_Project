/**
 * @file oled_ssd1306.h
 * @brief 128x64 I2C SSD1306 OLED Driver for STM32 Blue Pill
 *        Backed by Adafruit_SSD1306 on hardware with unit test fallback.
 */

#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdint.h>
#include <stddef.h>

#define OLED_WIDTH    128
#define OLED_HEIGHT   64
#define OLED_I2C_ADDR 0x3C  // Standard 0.96" OLED I2C address (0x3C or 0x3D)

class SSD1306Display {
public:
    SSD1306Display();

    // Initialize I2C communication and OLED display controller
    bool begin(uint8_t i2c_addr = OLED_I2C_ADDR);

    // Framebuffer operations
    void clear();
    void display();
    void setPixel(int16_t x, int16_t y, uint8_t color);

    // Drawing primitives
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t progress_percent);

    // Text output
    void setCursor(int16_t x, int16_t y);
    void setTextColor(uint8_t color); // 1 = White, 0 = Black (inverted)
    void setTextSize(uint8_t size);   // 1 = normal (6x8), 2 = double (12x16)
    void print(char c);
    void print(const char *text);
    void print(int value);

private:
    uint8_t _addr;
    int16_t _cursor_x;
    int16_t _cursor_y;
    uint8_t _text_color;
    uint8_t _text_size;
    uint8_t _buffer[OLED_WIDTH * OLED_HEIGHT / 8];
};

extern SSD1306Display OLED;

#endif // OLED_SSD1306_H
