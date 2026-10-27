/**
 * @file rotary_encoder.cpp
 * @brief EC11 / KY-040 Rotary Encoder Implementation
 */

#include "rotary_encoder.h"

#if defined(ARDUINO)
#ifndef GCC_VERSION
#define GCC_VERSION 140201
#endif
#include <Arduino.h>
#endif

RotaryEncoder::RotaryEncoder(uint8_t pin_clk, uint8_t pin_dt, uint8_t pin_sw)
    : _pin_clk(pin_clk), _pin_dt(pin_dt), _pin_sw(pin_sw),
      _prev_state(3), _sub_step(0), _rotation_delta(0),
      _btn_is_down(false), _btn_clicked(false), _btn_held_triggered(false),
      _btn_debounce_time(0), _btn_press_start(0) {
}

void RotaryEncoder::begin() {
#if defined(ARDUINO)
    pinMode(_pin_clk, INPUT_PULLUP);
    pinMode(_pin_dt, INPUT_PULLUP);
    pinMode(_pin_sw, INPUT_PULLUP);
    uint8_t clk = digitalRead(_pin_clk);
    uint8_t dt  = digitalRead(_pin_dt);
    _prev_state = (clk << 1) | dt;
    _btn_is_down = (digitalRead(_pin_sw) == LOW);
#endif
}

void RotaryEncoder::update() {
#if defined(ARDUINO)
    // 1. Gray-code quadrature transition decoding
    uint8_t clk = digitalRead(_pin_clk);
    uint8_t dt  = digitalRead(_pin_dt);
    uint8_t curr_state = (clk << 1) | dt;

    if (curr_state != _prev_state) {
        // 16-element quadrature state transition table:
        // Index: (prev_state << 2) | curr_state
        // Valid CW steps (+1), Valid CCW steps (-1), Invalid/no-op (0)
        static const int8_t QUAD_TABLE[16] = {
             0, -1,  1,  0,  // from 0 to [0, 1, 2, 3]
             1,  0,  0, -1,  // from 1 to [0, 1, 2, 3]
            -1,  0,  0,  1,  // from 2 to [0, 1, 2, 3]
             0,  1, -1,  0   // from 3 to [0, 1, 2, 3]
        };

        uint8_t idx = ((_prev_state & 0x03) << 2) | (curr_state & 0x03);
        int8_t step = QUAD_TABLE[idx];
        _sub_step += step;

        // Quadrature accumulation: 4 transitions per physical detent cycle
        if (_sub_step >= 4) {
            _rotation_delta += 1;
            _sub_step -= 4;
        } else if (_sub_step <= -4) {
            _rotation_delta -= 1;
            _sub_step += 4;
        } else if (curr_state == 3) {
            // At mechanical detent rest position (both lines pulled HIGH)
            if (_sub_step >= 2) {
                _rotation_delta += 1;
            } else if (_sub_step <= -2) {
                _rotation_delta -= 1;
            }
            _sub_step = 0;
        }

        _prev_state = curr_state;
    }

    // 2. Responsive push button reading with debounce and hold detection
    uint8_t raw_btn = digitalRead(_pin_sw); // Active LOW: 0 = pressed, 1 = released
    uint32_t now = millis();

    if (raw_btn == LOW) {
        if (!_btn_is_down && (now - _btn_debounce_time >= 40)) {
            _btn_is_down = true;
            _btn_press_start = now;
            _btn_held_triggered = false;
            _btn_debounce_time = now;
        }
    } else {
        if (_btn_is_down && (now - _btn_debounce_time >= 40)) {
            if (!_btn_held_triggered && (now - _btn_press_start >= 35)) {
                _btn_clicked = true; // Trigger click on release if not held
            }
            _btn_is_down = false;
            _btn_held_triggered = false;
            _btn_debounce_time = now;
        }
    }
#endif
}

int8_t RotaryEncoder::getRotation() {
    int8_t delta = _rotation_delta;
    _rotation_delta = 0;
    return delta;
}

bool RotaryEncoder::isClicked() {
    if (_btn_clicked) {
        _btn_clicked = false;
        return true;
    }
    return false;
}

bool RotaryEncoder::isHeld(uint32_t hold_ms) {
#if defined(ARDUINO)
    if (_btn_is_down && !_btn_held_triggered && (millis() - _btn_press_start >= hold_ms)) {
        _btn_held_triggered = true;
        return true;
    }
#else
    (void)hold_ms;
#endif
    return false;
}
