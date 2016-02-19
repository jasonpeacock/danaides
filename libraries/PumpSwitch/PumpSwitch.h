#ifndef PumpSwitch_h
#define PumpSwitch_h

// system
#include <Arduino.h>

// third-party
#include "Bounce2.h"

// local
#include "Danaides.h"
#include "Data.h"
#include "LED.h"

/*
 * Constants
 */

// pump switch dynamic data indexes
#define PUMP_VALUES_STATE   0
#define PUMP_VALUES_SECONDS 1
#define PUMP_VALUES_MINUTES 2
#define PUMP_VALUES_HOURS   3
#define PUMP_VALUES_DAYS    4

#define PUMP_VALUES_TOTAL 5

// pump switch configurable data indexes
#define PUMP_SETTINGS_MAX_ON_MINUTES   0
#define PUMP_SETTINGS_MIN_ON_MINUTES   1
#define PUMP_SETTINGS_MIN_OFF_MINUTES  2

#define PUMP_SETTINGS_TOTAL 3

class PumpSwitch {
    private:

        bool _master;

        bool _valuesInitialized;
        bool _settingsInitialized;

        Bounce _debouncer;
        uint8_t _buttonPin;

        void (*_startCallback)();
        void (*_stopCallback)();

        LED _led;

        uint8_t _values[PUMP_VALUES_TOTAL];
        uint8_t _settings[PUMP_SETTINGS_TOTAL];

        uint32_t _time;

        uint32_t _msToMinutes(uint32_t ms);
        void _resetValues(bool running);
        void _resetSettings();
        void _on();
        void _off();
        void _updateElapsedTime();

        uint32_t _calculateElapsedSeconds(uint8_t* values);

    public:
        PumpSwitch(bool master, uint8_t buttonPin, uint8_t ledPin, void (*startCallback)(), void (*stopCallback)());
        ~PumpSwitch();

        void setup();
        void check();

        bool isMaster();
        bool isSlave();

        bool isOn();
        bool isOff();

        // dynamic values
        void     updateValues(uint8_t *values, uint8_t numValues);
        uint8_t* getValues();
        uint8_t  getNumValues();
        uint8_t  getValueSeconds();
        uint8_t  getValueMinutes();
        uint8_t  getValueHours();
        uint8_t  getValueDays();
        uint32_t getElapsedSeconds();
        uint32_t getElapsedMinutes();

        // configuration settings
        void     updateSettings(uint8_t *settings, uint8_t numSettings);
        uint8_t* getSettings();
        uint8_t  getNumSettings();
        uint8_t  getMaxOnMinutes();
        uint8_t  getMinOnMinutes();
        uint8_t  getMinOffMinutes();

        void setLongOffMinutes(bool enabled);
        void setLongOnMinutes(bool enabled);

        bool start(bool force = false);
        bool  stop(bool force = false);

        bool ready();
};

#endif //PumpSwitch_h
