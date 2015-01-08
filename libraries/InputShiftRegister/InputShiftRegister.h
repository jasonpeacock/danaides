#ifndef InputShiftRegister_h
#define InputShiftRegister_h

// system
#include <Arduino.h>

/*
 * Constants
 */

// Clock Pulse width (per the docs)
#define PULSE_WIDTH_USEC 5

class InputShiftRegister {
    private:
        uint8_t _numInputs; // total number of shift register inputs, should
                            // be a multiple of 8 and <255
        uint8_t _plPin;     // Parallel Load pin
        uint8_t _cePin;     // Clock Enable pin
        uint8_t _cpPin;     // Clock Pulse pin
        uint8_t _q7Pin;     // Serial Out (Q7) pin

    public:
        InputShiftRegister(uint8_t numInputs, uint8_t plPin, uint8_t cePin, uint8_t cpPin, uint8_t q7Pin);
        ~InputShiftRegister();

        // to be called during setup() in main Arduino sketch,
        // will setup the pins properly and anything else.
        void setup();

        // retrieve all the digital input values
        bool getValues(uint8_t* values);

        // how many inputs do we think we have?
        uint8_t getNumInputs();
};

#endif //InputShiftRegister_h
