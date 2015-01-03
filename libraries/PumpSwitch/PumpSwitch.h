#ifndef PumpSwitch_h
#define PumpSwitch_h

#include <Arduino.h>

#include "Bounce2.h"

#include "LED.h"

/*
 * Constants
 */

//XXX adjust these to actual values (60min each)
#define PUMP_DEFAULT_MAX_ON_MINUTES  2
#define PUMP_DEFAULT_MIN_OFF_MINUTES 2

#define PUMP_START_ATTEMPTS_WINDOW_SECONDS 60
#define PUMP_MIN_START_ATTEMPTS 3

// pump switch outgoing data
#define PUMP_STATE   0
#define PUMP_SECONDS 1
#define PUMP_MINUTES 2
#define PUMP_HOURS   3
#define PUMP_DAYS    4

#define PUMP_TOTAL_VALUES 5

class PumpSwitch {
    private:

        Bounce _debouncer;
        int _buttonPin;

        void (*_startCallback)();
        void (*_stopCallback)();

        LED _led;

        uint8_t _pumpValues[PUMP_TOTAL_VALUES];

        uint32_t _time;

        uint32_t _lastStartAttemptTime;
        int _startAttemptCount;

        int _maxOnMinutes;
        int _minOffMinutes;

        uint32_t _msToMinutes(uint32_t ms);
        void _resetPumpValues(bool running);
        void _on();
        void _off();
        void _updateElapsedTime();

    public:
        PumpSwitch(int buttonPin, int ledPin, void (*startCallback)(), void (*stopCallback)());
        ~PumpSwitch();

        // to be called during setup() in main Arduino sketch,
        // will setup the pins properly and anything else.
        void setup();

        bool isOn();
        bool isOff();

        uint8_t* getPumpValues();
        int getMaxOnMinutes();
        int getMinOffMinutes();
        uint32_t getElapsedSeconds();

        void check();
        void start();
        void stop();
};

#endif //PumpSwitch_h
