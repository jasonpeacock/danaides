#ifndef PumpSwitch_h
#define PumpSwitch_h

// system
#include <Arduino.h>

// third-party
#include "Bounce2.h"

// local
#include "Data.h"
#include "LED.h"

/*
 * Constants
 */

//XXX adjust these to actual values (60min each)
#define PUMP_DEFAULT_MAX_ON_MINUTES  2
#define PUMP_DEFAULT_MIN_OFF_MINUTES 2

#define PUMP_START_ATTEMPTS_WINDOW_SECONDS 60
#define PUMP_MIN_START_ATTEMPTS 3

/*
 * Array Indexes
 */

// pump switch dynamic data
#define PUMP_VALUES_STATE   0
#define PUMP_VALUES_SECONDS 1
#define PUMP_VALUES_MINUTES 2
#define PUMP_VALUES_HOURS   3
#define PUMP_VALUES_DAYS    4

#define PUMP_VALUES_TOTAL 5

// pump switch configurable data
#define PUMP_SETTINGS_MAX_ON_MINUTES  0
#define PUMP_SETTINGS_MIN_OFF_MINUTES 1

#define PUMP_SETTINGS_TOTAL 2

class PumpSwitch {
    private:

        Bounce _debouncer;
        uint8_t _buttonPin;

        void (*_startCallback)();
        void (*_stopCallback)();

        LED _led;

        uint8_t _values[PUMP_VALUES_TOTAL];
        uint8_t _settings[PUMP_SETTINGS_TOTAL];

        uint32_t _time;

        uint32_t _lastStartAttemptTime;
        uint8_t _startAttemptCount;

        uint32_t _msToMinutes(uint32_t ms);
        void _resetValues(bool running);
        void _resetSettings();
        void _on();
        void _off();
        void _updateElapsedTime();

    public:
        PumpSwitch(uint8_t buttonPin, uint8_t ledPin, void (*startCallback)(), void (*stopCallback)());
        ~PumpSwitch();

        // to be called during setup() in main Arduino sketch,
        // will setup the pins properly and anything else.
        void setup();

        bool isOn();
        bool isOff();

        // dynamic values
        void     updateValues(uint8_t *values, uint8_t numValues);
        uint8_t* getValues();
        uint8_t  getNumValues();
        uint32_t getElapsedSeconds();

        // configuration settings
        void     updateSettings(uint8_t *settings, uint8_t numSettings);
        uint8_t* getSettings();
        uint8_t  getNumSettings();
        uint8_t  getMaxOnMinutes();
        uint8_t  getMinOffMinutes();


        void check();
        void start(bool force = false);
        void stop();
};

#endif //PumpSwitch_h
