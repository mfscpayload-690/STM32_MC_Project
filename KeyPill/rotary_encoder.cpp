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
      _last_clk_state(1), _rotation_delta(0),
      _btn_last_state(1), _btn_clicked(false), _btn_press_start(0), _btn_is_down(false) {
}

void RotaryEncoder::begin() {
#if defined(ARDUINO)
    pinMode(_pin_clk, INPUT_PULLUP);
    pinMode(_pin_dt, INPUT_PULLUP);
    pinMode(_pin_sw, INPUT_PULLUP);
    _last_clk_state = digitalRead(_pin_clk);
    _btn_last_state = digitalRead(_pin_sw);
#endif
}

void RotaryEncoder::update() {
#if defined(ARDUINO)
    // 1. Read quadrature rotation
    uint8_t current_clk = digitalRead(_pin_clk);
    if (current_clk != _last_clk_state && current_clk == LOW) {
        // Falling edge on CLK: determine direction by checking DT
        uint8_t current_dt = digitalRead(_pin_dt);
        if (current_dt != current_clk) {
            _rotation_delta += 1; // Clockwise
        } else {
            _rotation_delta -= 1; // Counter-clockwise
        }
    }
    _last_clk_state = current_clk;

    // 2. Read switch with software debounce
    uint8_t btn_current = digitalRead(_pin_sw); // Active LOW
    uint32_t now = millis();

    if (btn_current == LOW && _btn_last_state == HIGH) {
        // Pressed down
        _btn_press_start = now;
        _btn_is_down = true;
    } else if (btn_current == HIGH && _btn_last_state == LOW) {
        // Released
        if (_btn_is_down && (now - _btn_press_start >= 35)) {
            _btn_clicked = true;
        }
        _btn_is_down = false;
    }
    _btn_last_state = btn_current;
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
    if (_btn_is_down && (millis() - _btn_press_start >= hold_ms)) {
        return true;
    }
#else
    (void)hold_ms;
#endif
    return false;
}
