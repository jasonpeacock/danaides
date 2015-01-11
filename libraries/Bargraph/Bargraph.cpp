// system
#include <Arduino.h>

// third-party
#include "Adafruit_LEDBackpack.h"

// local
#include "Bargraph.h"

/*
 * Private
 */

/*
 * Public
 */

Bargraph::Bargraph() : 
    _bar(Adafruit_24bargraph()),
    _address(0),
    _lastUpdateTime(0UL) {
}

Bargraph::Bargraph(uint8_t address) : 
    _bar(Adafruit_24bargraph()),
    _address(address),
    _lastUpdateTime(0UL) {
}

Bargraph::~Bargraph() {
}

void Bargraph::setup() {
    _bar.begin(_address);
    reset();
}

void Bargraph::reset() {
    _bar.clear();
    _bar.writeDisplay();
}

void Bargraph::check() {
    if (!_lastUpdateTime || millis() - _lastUpdateTime > BARGRAPH_UPDATE_INTERVAL_SECONDS * 1000UL) {
        _lastUpdateTime = millis();

        _bar.writeDisplay();
    }
} 

void Bargraph::setBlock(bool state, bool fill, uint8_t block, uint8_t totalBlocks) {
    uint8_t blockSize = (uint8_t)(BARGRAPH_TOTAL_BARS / totalBlocks);
    uint8_t startBlock = 0 + block * blockSize;
    uint8_t endBlock = startBlock + blockSize;

    // fill in the bar section (or not)
    for (uint8_t i = startBlock; i < endBlock; i++) {
        if (fill) {
            _bar.setBar(i, LED_YELLOW);
        } else {
            _bar.setBar(i, LED_OFF);
        }
    }

    // color the marker (end of block)
    if (state) {
        _bar.setBar(startBlock, LED_GREEN);
    } else {
        _bar.setBar(startBlock, LED_RED);
    }
}

