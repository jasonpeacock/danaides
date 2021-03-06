// system
#include <Arduino.h>

// local
#include "Data.h"
#include "InputShiftRegister.h"

InputShiftRegister::InputShiftRegister(uint8_t numInputs, 
                                       uint8_t plPin, 
                                       uint8_t cePin, uint8_t cpPin, 
                                       uint8_t q7Pin) : 
                                       _numInputs(numInputs), 
                                       _plPin(plPin), 
                                       _cePin(cePin), 
                                       _cpPin(cpPin), 
                                       _q7Pin(q7Pin) {
}

InputShiftRegister::~InputShiftRegister() {
}

void InputShiftRegister::setup() {
    pinMode(_plPin, OUTPUT);
    pinMode(_cePin, OUTPUT);
    pinMode(_cpPin, OUTPUT);
    pinMode(_q7Pin, INPUT);

    // init the shift register
    digitalWrite(_cpPin, LOW);
    digitalWrite(_plPin, HIGH);
}

void InputShiftRegister::getInputValues(Data &data) {
    // 1. disable the clock to prevent reading
    // 2. read the inputs w/parallel load, pausing
    //    long enough to capture all the values
    // 3. re-enable the clock to read the values
    digitalWrite(_cePin, HIGH);
    digitalWrite(_plPin, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(_plPin, HIGH);
    digitalWrite(_cePin, LOW);

    uint8_t values[_numInputs];
    for (uint8_t i = 0; i < _numInputs; i++) {
        // order of values returned from the shift register
        // is from 32 -> 1, so need to revese & offset them
        // for index use.
        uint8_t index = _numInputs - i - 1;
        
        values[index] = digitalRead(_q7Pin);

        // pulse the clock to load the next input value
        digitalWrite(_cpPin, HIGH);
        delayMicroseconds(PULSE_WIDTH_USEC);
        digitalWrite(_cpPin, LOW);
    }

    data.set(values, _numInputs);
}

uint8_t InputShiftRegister::getNumInputs() {
    return _numInputs;
}

