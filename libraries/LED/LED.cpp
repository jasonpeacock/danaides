// system
#include <Arduino.h>

#include "LED.h"

LED::LED() :
    _pin(0),
    _on(false),
    _currLedState(false),
    _maxFlashCount(0),
    _currFlashCount(0),
    _flashDelayMs(0UL),
    _lastFlashTime(0UL) {

    _resetHeartbeat();
}

LED::LED(uint8_t pin) :
    _pin(pin),
    _on(false),
    _currLedState(false),
    _maxFlashCount(0),
    _currFlashCount(0),
    _flashDelayMs(0UL),
    _lastFlashTime(0UL) {

    _resetHeartbeat();
}

LED::~LED() {
}

void LED::_resetHeartbeat() {
    _lastHeartbeatTime = millis();
}

void LED::_checkFlash() {
    if (!_lastFlashTime || millis() - _lastFlashTime > _flashDelayMs) {
        if (_currFlashCount < _maxFlashCount) {
            _lastFlashTime = millis();
            _currFlashCount++;

            if (_currLedState) {
                digitalWrite(_pin, LOW);
                _currLedState = false;
            } else {
                digitalWrite(_pin, HIGH);
                _currLedState = true;
            }

            // delay the heartbeat if we've recently flashed
            _resetHeartbeat();
        } else {
            // after flashing, return to desired state
            if (_on && !_currLedState) {
                digitalWrite(_pin, HIGH);
                _currLedState = true;
            } else if (!_on && _currLedState) {
                digitalWrite(_pin, LOW);
                _currLedState = false;
            }
        }
    }
}

void LED::_checkFlashing() {
    if (!_lastFlashTime || millis() - _lastFlashTime > LED_FLASHING_INTERVAL) {
        _lastFlashTime = millis();

        if (_currLedState) {
            digitalWrite(_pin, LOW);
            _currLedState = false;
        } else {
            digitalWrite(_pin, HIGH);
            _currLedState = true;
        }

        // delay the heartbeat if we've recently flashed
        _resetHeartbeat();
    }
}

void LED::setup() {
    if (!enabled()) {
        return;
    }

    pinMode(_pin, OUTPUT);

    // start with the LED off
    digitalWrite(_pin, LOW);
    _currLedState = false;
    _resetHeartbeat();
}

void LED::check() {
    if (!enabled()) {
        return;
    }

    if (_flashing) {
        _checkFlashing();
    } else {
        _checkFlash();
    }
}

bool LED::enabled() {
    return _pin;
}

// idempotent, don't check current state
void LED::on() {
    _on = true;

    if (!enabled()) {
        return;
    }

    digitalWrite(_pin, HIGH);
    _currLedState = true;
    _resetHeartbeat();
}

// idempotent, don't check current state
void LED::off() {
    _on = false;

    if (!enabled()) {
        return;
    }

    digitalWrite(_pin, LOW);
    _currLedState = false;
    _resetHeartbeat();
}

// flash/blink the LED, based on current button LED state
void LED::flash(uint8_t times, uint32_t wait) {
    _maxFlashCount = times * 2; // account for on AND off cycles of flash
    _flashDelayMs = wait;
    _lastFlashTime = 0UL;
    _currFlashCount = 0;
}

// how to know if done flashing b/c it's asynchronous
bool LED::completedFlashing() {
    if (!enabled()) {
        // disabled will never start flashing
        return true;
    } else if (_flashing) {
        // is flashing forever until explicitly stopped
        return false;
    } else if (enabled() && _currFlashCount >= _maxFlashCount) {
        return true;
    } else {
        return false;
    }
}

/*
 * Flash convenience methods
 */

// flash forever
void LED::flashing(bool enable) {
    _flashing = enable;
}

// show a "thinking" LED
void LED::thinking() {
    flash(5, 100);
}

// show success/assent
void LED::success() {
    flash(1, 250);
}

// show error/refusal
void LED::error() {
    flash(3, 500);
}

// show "I'm alive"
void LED::heartbeat() {
    if (millis() - _lastHeartbeatTime > LED_HEARTBEAT_INTERVAL_SECONDS * 1000LU) {
        flash(1, 250);
    }
}


