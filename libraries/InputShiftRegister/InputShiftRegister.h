#ifndef InputShiftRegsiter_h
#define InputShiftRegister_h

/*
 * Constants
 */

// Clock Pulse width (per the docs)
#define PULSE_WIDTH_USEC 5

class InputShiftRegister {
    private:
        int _numInputs; // total number of shift register inputs, should
                        // be a multiple of 8.
        int _plPin;     // Parallel Load pin
        int _cePin;     // Clock Enable pin
        int _cpPin;     // Clock Pulse pin
        int _q7Pin;     // Serial Out (Q7) pin

        int* _values;   // values to be read from the shift register

    public:
        InputShiftRegister(int numInputs, int plPin, int cePin, int cpPin, int q7Pin);
        ~InputShiftRegister();

        // to be called during setup() in main Arduino sketch,
        // will setup the pins properly and anything else.
        void setup();

        // retrieve all the digital input values
        int* getValues();

        // how many inputs do we think we have?
        int getNumInputs();
};

#endif //InputShiftRegister_h
