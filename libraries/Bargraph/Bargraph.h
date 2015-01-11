#ifndef Bargraph_h
#define Bargraph_h

// system
#include <Arduino.h>

// third-party
#include "Adafruit_LEDBackpack.h"

/*
 * Constants
 */

#define BARGRAPH_TOTAL_BARS 24
#define BARGRAPH_UPDATE_INTERVAL_SECONDS 5UL

class Bargraph {
    private:
        Adafruit_24bargraph _bar;
        uint8_t _address;

        uint8_t _totalBlocks;

        uint32_t _lastUpdateTime;

    public:
        Bargraph();
        Bargraph(uint8_t address, uint8_t totalBlocks);
        ~Bargraph();

        void setup();
        void check();

        void reset();

        void setBlock(bool state, bool fill, uint8_t block);
};

#endif //Bargraph_h
