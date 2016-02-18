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
    _totalBlocks(0),
    _lastUpdateTime(0UL) {
}

Bargraph::Bargraph(uint8_t address, uint8_t totalBlocks) : 
    _bar(Adafruit_24bargraph()),
    _address(address),
    _totalBlocks(totalBlocks),
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

// block "0" should be at the top of the bar (highest values)
void Bargraph::setBlock(bool state, bool fill, uint8_t block) {
    uint8_t blockSize = (uint8_t)(BARGRAPH_TOTAL_BARS / _totalBlocks);
    uint8_t startBlock = BARGRAPH_TOTAL_BARS - blockSize - block * blockSize;
    uint8_t endBlock = startBlock + blockSize - 1;

    // fill in the bar
    for (uint8_t i = startBlock; i < endBlock; i++) {
        if (fill && state) {
            // make the fill green if it's ON
            _bar.setBar(i, LED_GREEN);
        } else if (fill) {
            // make it yellow if it's
            // below an ON block
            _bar.setBar(i, LED_YELLOW);
        } else {
            // leave it empty if above/none ON blocks
            _bar.setBar(i, LED_OFF);
        }
    }

    // color the marker (end of block)
    if (state) {
        _bar.setBar(endBlock, LED_YELLOW);
    } else {
        _bar.setBar(endBlock, LED_RED);
    }
}

