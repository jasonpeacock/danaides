// system
#include <Arduino.h>
#include <inttypes.h>

#include "LED.h"

LED::LED() {
    _init(0);
}

LED::LED(uint8_t pin) {
    _init(pin);
}

LED::~LED() {
}

void LED::_resetHeartbeat() {
    _lastHeartbeatTime = millis();
}

void LED::_init(uint8_t pin) {
    _pin = pin;
    _on = false;
    _resetHeartbeat();
}

void LED::setup() {
    if (!enabled()) {
        return;
    }

    pinMode(_pin, OUTPUT);

    // start with the LED off
    digitalWrite(_pin, LOW);
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
    _resetHeartbeat();
}

// idempotent, don't check current state
void LED::off() {
    _on = false;

    if (!enabled()) {
        return;
    }

    digitalWrite(_pin, LOW);
    _resetHeartbeat();
}

// flash/blink the LED, based on current button LED state
void LED::flash(uint8_t times, uint16_t wait) {
    if (!enabled()) {
        return;
    }

    for (uint8_t i = 0; i < times; i++) {
        _on ? digitalWrite(_pin, LOW) : digitalWrite(_pin, HIGH);
        delay(wait);
        _on ? digitalWrite(_pin, HIGH) : digitalWrite(_pin, LOW);

        if (i + 1 < times) {
            delay(wait);
        }
    }
    _resetHeartbeat();
}

/*
 * Flash convenience methods
 */

// show a "thinking" LED
void LED::thinking() {
    flash(5, 100);
    _resetHeartbeat();
}

// show error/refusal
void LED::error() {
    flash(3, 500);
    _resetHeartbeat();
}

// show "I'm alive"
void LED::heartbeat() {
    if (millis() - _lastHeartbeatTime > LED_HEARTBEAT_INTERVAL_SECONDS * 1000LU) {
        flash(1, 250);
        _resetHeartbeat();
    }
}


