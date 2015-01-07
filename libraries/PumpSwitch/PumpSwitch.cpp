// local
#include "PumpSwitch.h"

/*
 * Private
 */

uint32_t PumpSwitch::_msToMinutes(uint32_t ms) {
    return (ms / 1000) / 60;
}

void PumpSwitch::_resetValues(bool running) {
    _time = millis();
    _values[PUMP_VALUES_STATE]   = running;
    _values[PUMP_VALUES_DAYS]    = 0;
    _values[PUMP_VALUES_HOURS]   = 0;
    _values[PUMP_VALUES_MINUTES] = 0;
    _values[PUMP_VALUES_SECONDS] = 0;
}

void PumpSwitch::_resetSettings() {
    _settings[PUMP_SETTINGS_ON_DELAY_MINUTES] = PUMP_DEFAULT_ON_DELAY_MINUTES;
    _settings[PUMP_SETTINGS_MAX_ON_MINUTES]   = PUMP_DEFAULT_MAX_ON_MINUTES;
    _settings[PUMP_SETTINGS_MIN_ON_MINUTES]   = PUMP_DEFAULT_MIN_ON_MINUTES;
    _settings[PUMP_SETTINGS_MIN_OFF_MINUTES]  = PUMP_DEFAULT_MIN_OFF_MINUTES;
}

void PumpSwitch::_on() {
    _resetValues(true);
}

void PumpSwitch::_off() {
    _resetValues(false);
}

/*
 * Calculate the elapsed time since the pump started/stopped
 * as Days, Hours, Minutes, Seconds and update the values.
 *
 * The Days & Hours are calculated for the values because
 * the pump may be off for multiple hours/days and we need to inform
 * the base station.
 */
void PumpSwitch::_updateElapsedTime() {
    if (millis() - _time >= 1000UL) {
        _values[PUMP_VALUES_SECONDS] += (millis() - _time) / 1000;

        _values[PUMP_VALUES_MINUTES] += _values[PUMP_VALUES_SECONDS] / 60;
        _values[PUMP_VALUES_SECONDS]  = _values[PUMP_VALUES_SECONDS] % 60;

        _values[PUMP_VALUES_HOURS] += _values[PUMP_VALUES_MINUTES] / 60;
        _values[PUMP_VALUES_MINUTES]  = _values[PUMP_VALUES_MINUTES] % 60;

        _values[PUMP_VALUES_DAYS] += _values[PUMP_VALUES_HOURS] / 24;
        _values[PUMP_VALUES_HOURS]  = _values[PUMP_VALUES_HOURS] % 24;

        _time = millis();
    }
}

/*
 * Public
 */

PumpSwitch::PumpSwitch(uint8_t buttonPin, uint8_t ledPin, void (*startCallback)(), void (*stopCallback)()) {
    _debouncer = Bounce(); 
    _buttonPin = buttonPin;

    _led = LED(ledPin);

    _startCallback = startCallback;
    _stopCallback = stopCallback;

    _time = millis();

    _lastStartAttemptTime = 0;
    _startAttemptCount = 0;

    // start with the pump off
    _off();

    // start with default settings
    _resetSettings();
}

PumpSwitch::~PumpSwitch() {
}

void PumpSwitch::setup() {
    pinMode(_buttonPin, INPUT_PULLUP);
    _debouncer.attach(_buttonPin);
    _debouncer.interval(5); // ms

    _led.setup();
}

bool PumpSwitch::isOn() {
    return _values[PUMP_VALUES_STATE] ? true : false;
}

bool PumpSwitch::isOff() {
    return !isOn();
}

void PumpSwitch::updateValues(uint8_t *values, uint8_t numValues) {
    if (PUMP_VALUES_TOTAL != numValues) {
        Serial.println(F("[ERROR] updateValues: PUMP_VALUES_TOTAL != numValues"));
        return;
    }

    if (_values[PUMP_VALUES_STATE] != values[PUMP_VALUES_STATE]) {
        Serial.println(F("New values have a different pump state, updating Pump Switch"));
        // the updated values reflect a different pump state,
        // update (force) our Pump Switch to match
        values[PUMP_VALUES_STATE] ? start(true) : stop();
    }

    // and now copy all the values to match (especially the elapsed times)
    for (uint8_t i = 0; i < numValues; i++) {
        _values[i] = values[i];
    }

    // then reset the elapsed time counter
    _time = millis();
}

uint8_t* PumpSwitch::getValues() {
    return _values;
}

uint8_t PumpSwitch::getNumValues() {
    return PUMP_VALUES_TOTAL;
}

void PumpSwitch::updateSettings(uint8_t *settings, uint8_t numSettings) {
    if (PUMP_SETTINGS_TOTAL != numSettings) {
        Serial.println(F("[ERROR] updateSettings: numSettings != PUMP_SETTINGS_TOTAL"));
        return;
    }

    for (uint8_t i = 0; i < numSettings; i++) {
        _settings[i] = settings[i];
    }
}

uint8_t* PumpSwitch::getSettings() {
    return _settings;
}

uint8_t PumpSwitch::getNumSettings() {
    return PUMP_SETTINGS_TOTAL;
}

uint8_t PumpSwitch::getOnDelayMinutes() {
    return _settings[PUMP_SETTINGS_ON_DELAY_MINUTES];
}

uint8_t PumpSwitch::getMaxOnMinutes() {
    return _settings[PUMP_SETTINGS_MAX_ON_MINUTES];
}

uint8_t PumpSwitch::getMinOnMinutes() {
    return _settings[PUMP_SETTINGS_MIN_ON_MINUTES];
}

uint8_t PumpSwitch::getMinOffMinutes() {
    return _settings[PUMP_SETTINGS_MIN_OFF_MINUTES];
}

uint32_t PumpSwitch::getElapsedSeconds() {
    uint32_t seconds = 0UL;
    seconds += _values[PUMP_VALUES_SECONDS];
    seconds += _values[PUMP_VALUES_MINUTES] * 60UL;
    seconds +=   _values[PUMP_VALUES_HOURS] * 60UL * 60UL;
    seconds +=    _values[PUMP_VALUES_DAYS] * 60UL * 60UL * 24UL;

    return seconds;
}

uint32_t PumpSwitch::getElapsedMinutes() {
    return getElapsedSeconds() / 60L;
}

/*
 * Check how long the pump has been running and 
 * stop the pump if it exceeds the limit.
 */
void PumpSwitch::check() {
    _updateElapsedTime();

    if (_debouncer.update() && _debouncer.fell()) {
        // button was pressed
        isOn() ? stop() : start();
    }

    if (isOn()) {
        if (getMaxOnMinutes() <= getElapsedMinutes()) {
            // ran too long, time to stop the pump
            stop();
        } else {
            // do nothing, let the pump continue running
        }
    } else {
        // do nothing, let the pump stay off
    }

    // regardless of pump state, always blink/flash
    _led.heartbeat();
}

/*
 * Start the pump, triggered either by user input (button)
 * or via request from the base station.
 */
void PumpSwitch::start(bool force) {
    Serial.print(F("Starting pump for "));
    Serial.print(getMaxOnMinutes());
    Serial.println(F("min"));

    if (isOn()) {
        Serial.print(F("Pump already running for "));
        Serial.print(getElapsedMinutes());
        Serial.println(F("min"));
        // don't do anything if the pump is already running
        return;
    }

    // don't start the pump if it hasn't rested long enough
    if (getMinOffMinutes() > getElapsedMinutes()) {
        if (!_lastStartAttemptTime || millis() - _lastStartAttemptTime < PUMP_START_ATTEMPTS_WINDOW_SECONDS * 1000LU) {
            _startAttemptCount++;
        } else {
            _lastStartAttemptTime = millis();
            _startAttemptCount = 1;
        }

        if (!force && PUMP_MIN_START_ATTEMPTS > _startAttemptCount) {
            Serial.print(F("Pump only off "));
            Serial.print(getElapsedMinutes());
            Serial.print(F("min (MINIMUM: "));
            Serial.print(getMinOffMinutes());
            Serial.println(F("min), NOT starting"));

            _led.error();
            return;
        } else {
            Serial.println(F("Pump override enabled!"));

            // reset the attempt counter/time
            _lastStartAttemptTime = 0;
            _startAttemptCount = 0;
        }
    }

    _led.thinking();

    // turn on the LED
    _led.on();

    // set the internal state
    _on();

    // actually start the pump!
    (*_startCallback)();

    Serial.println(F("Pump started"));
}

/*
 * Stop the pump, triggered either by user input (button)
 * or via request from the base station.
 */
void PumpSwitch::stop() {
    Serial.print(F("Stopping pump, cannot restart for "));
    Serial.print(getMinOffMinutes());
    Serial.println(F("min"));

    if (isOff()) {
        Serial.print(F("Pump was already stopped "));
        Serial.print(getElapsedMinutes());
        Serial.println(F("min ago"));

        // don't do anything if the pump is already stopped
        return;
    }

    _led.thinking();

    // turn off the LED
    _led.off();

    // set the internal state
    _off();

    // actually stop the pump!
    (*_stopCallback)();

    Serial.println(F("Pump stopped"));
}

