#include <Arduino.h>

#include "Dbg.h"

#include "InputShiftRegister.h"

InputShiftRegister::InputShiftRegister(int numInputs, int plPin, int cePin, int cpPin, int q7Pin) {
    Debug.begin();

    _numInputs = numInputs;
    _plPin = plPin;
    _cePin = cePin;
    _cpPin = cpPin;
    _q7Pin = q7Pin;

    _values = new int[_numInputs];
}

InputShiftRegister::~InputShiftRegister() {
    delete _values;
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

int* InputShiftRegister::getValues() {
    // 1. disable the clock to prevent reading
    // 2. read the inputs w/parallel load, pausing
    //    long enough to capture all the values
    // 3. re-enable the clock to read the values
    digitalWrite(_cePin, HIGH);
    digitalWrite(_plPin, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(_plPin, HIGH);
    digitalWrite(_cePin, LOW);

    for (int i = (_numInputs - 1); i >= 0; i--) {
        _values[i] = digitalRead(_q7Pin);

        // pulse the clock to load the next input value
        digitalWrite(_cpPin, HIGH);
        delayMicroseconds(PULSE_WIDTH_USEC);
        digitalWrite(_cpPin, LOW);
    }

    return _values;
}

int InputShiftRegister::getNumInputs() {
    return _numInputs;
}

