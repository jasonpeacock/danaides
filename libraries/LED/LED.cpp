#include <Arduino.h>

#include "LED.h"

LED::LED() {
    _init(-1);
}

LED::LED(int pin) {
    _init(pin);
}

LED::~LED() {
}

void LED::_init(int pin) {
    _pin = pin;
    _lastHeartbeatTime = millis();
    _enabled = false;
}

void LED::setup() {
    pinMode(_pin, OUTPUT);

    // start with the LED off
    digitalWrite(_pin, LOW);
}

// idempotent, don't check current state
void LED::on() {
    _enabled = true;
    digitalWrite(_pin, HIGH);
}

// idempotent, don't check current state
void LED::off() {
    _enabled = false;
    digitalWrite(_pin, LOW);
}

// flash/blink the LED, based on current button LED state
void LED::flash(int times, int wait) {
    for (int i = 0; i < times; i++) {
        _enabled ? digitalWrite(_pin, LOW) : digitalWrite(_pin, HIGH);
        delay(wait);
        _enabled ? digitalWrite(_pin, HIGH) : digitalWrite(_pin, LOW);

        if (i + 1 < times) {
            delay(wait);
        }
    }
}

/*
 * Flash convenience methods
 */

// show a "thinking" LED
void LED::thinking() {
    flash(5, 100);
}

// show error/refusal
void LED::error() {
    flash(3, 500);
}

// show "I'm alive"
void LED::heartbeat() {
    if (millis() - _lastHeartbeatTime > LED_HEARTBEAT_INTERVAL_SECONDS * 1000LU) {
        _lastHeartbeatTime = millis();
        flash(1, 250);
    }
}


