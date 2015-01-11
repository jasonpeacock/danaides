// system
#include <Arduino.h>

// third-party
#include "Adafruit_LEDBackpack.h"

// local
#include "Counter.h"

/*
 * Private
 */

/*
 * Public
 */

Counter::Counter() : 
    _counter(Adafruit_7segment()),
    _address(0) {
}

Counter::Counter(uint8_t address) : 
    _counter(Adafruit_7segment()),
    _address(address) {
}

Counter::~Counter() {
}

void Counter::setup() {
    _counter.begin(_address);
    reset();
}

void Counter::reset() {
    _counter.clear();
    _counter.writeDisplay();
}

/*
 * use not-unsigned int so that we know if it's gone negative, 
 * if the count goes negative, start flashing "00:00"
 */
void Counter::check(bool show, int32_t seconds) {
    if (show) {
        // flash the colon every 0.5s
        bool drawColon = (millis() % 1000 < 500) ? true : false;
        _counter.drawColon(drawColon);

        if (0 < seconds) {
            uint32_t minutes = (uint32_t)(seconds / 60);
            _counter.writeDigitNum(0, minutes / 10, false);
            _counter.writeDigitNum(1, minutes % 10, false);

            uint32_t remainderSeconds = seconds % 60;
            _counter.writeDigitNum(3, remainderSeconds / 10, false);
            _counter.writeDigitNum(4, remainderSeconds % 10, false);
        } else {
            if (drawColon) {
                _counter.writeDigitNum(0, 0, false);
                _counter.writeDigitNum(1, 0, false);
                _counter.writeDigitNum(3, 0, false);
                _counter.writeDigitNum(4, 0, false);
            } else {
                _counter.clear();
            }
        }
        _counter.writeDisplay();
    } else {
        reset();
    }
}

