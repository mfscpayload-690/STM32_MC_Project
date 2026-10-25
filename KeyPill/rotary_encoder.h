/**
 * @file rotary_encoder.h
 * @brief EC11 / KY-040 Rotary Encoder Driver with Quadrature Decoding & Debounce
 */

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

class RotaryEncoder {
public:
    RotaryEncoder(uint8_t pin_clk, uint8_t pin_dt, uint8_t pin_sw);

    void begin();
    void update();

    // Returns +1 for clockwise step, -1 for counter-clockwise step, 0 for idle
    int8_t getRotation();

    // Returns true once when the center push switch is pressed and released
    bool isClicked();

    // Returns true if the center switch is held down for more than hold_ms
    bool isHeld(uint32_t hold_ms = 1500);

private:
    uint8_t _pin_clk;
    uint8_t _pin_dt;
    uint8_t _pin_sw;

    uint8_t _last_clk_state;
    int8_t  _rotation_delta;

    bool     _btn_last_state;
    bool     _btn_clicked;
    uint32_t _btn_press_start;
    bool     _btn_is_down;
};

#endif // ROTARY_ENCODER_H
