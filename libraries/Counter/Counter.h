#ifndef Counter_h
#define Counter_h

// system
#include <Arduino.h>

// third-party
#include "Adafruit_LEDBackpack.h"

/*
 * Constants
 */

class Counter {
    private:
        Adafruit_7segment _counter;
        uint8_t _address;

    public:
        Counter();
        Counter(uint8_t address);
        ~Counter();

        void setup();
        void check(bool show, int32_t seconds);

        void reset();
};

#endif //Counter_h
