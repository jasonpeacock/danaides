// local
#include "PumpSwitch.h"

/*
 * Private
 */

uint32_t PumpSwitch::_msToMinutes(uint32_t ms) {
    return (ms / 1000) / 60;
}

void PumpSwitch::_resetValues(bool running) {
    _values[PUMP_VALUES_STATE]   = running;
    _values[PUMP_VALUES_DAYS]    = 0;
    _values[PUMP_VALUES_HOURS]   = 0;
    _values[PUMP_VALUES_MINUTES] = 0;
    _values[PUMP_VALUES_SECONDS] = 0;
}

void PumpSwitch::_resetSettings() {
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

uint32_t PumpSwitch::_calculateElapsedSeconds(uint8_t* values) {
    uint32_t seconds = 0UL;
    seconds += values[PUMP_VALUES_SECONDS];
    seconds += values[PUMP_VALUES_MINUTES] * 60UL;
    seconds +=   values[PUMP_VALUES_HOURS] * 60UL * 60UL;
    seconds +=    values[PUMP_VALUES_DAYS] * 60UL * 60UL * 24UL;

    return seconds;
}

/*
 * Public
 */

PumpSwitch::PumpSwitch(bool master, 
                       uint8_t buttonPin, 
                       uint8_t ledPin, 
                       void (*startCallback)(), 
                       void (*stopCallback)()) :
    _master(master),
    _debouncer(Bounce()),
    _buttonPin(buttonPin),
    _led(LED(ledPin)),
    _startCallback(startCallback),
    _stopCallback(stopCallback),
    _time(millis()),
    _startAttemptTime(0UL),
    _startAttemptCount(0) {

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

bool PumpSwitch::isMaster() {
    return _master;
}

bool PumpSwitch::isSlave() {
    return !isMaster();
}

bool PumpSwitch::isOn() {
    return _values[PUMP_VALUES_STATE] ? true : false;
}

bool PumpSwitch::isOff() {
    return !isOn();
}

/*
 * Either slave or master can update values to start/stop the pump,
 * but only the master can correct (synchronize) the elapsed time.
 *
 */
void PumpSwitch::updateValues(uint8_t *values, uint8_t numValues) {
    if (PUMP_VALUES_TOTAL != numValues) {
        Serial.println(F("[ERROR] updateValues: PUMP_VALUES_TOTAL != numValues"));
        return;
    }

    if (_values[PUMP_VALUES_STATE] != values[PUMP_VALUES_STATE]) {
        Serial.println(F("New values have a different pump state, updating Pump Switch"));
        // the updated values reflect a different pump state,
        // update (force) our PumpSwitch to match
        values[PUMP_VALUES_STATE] ? start(true) : stop(true);
    }

    if (isSlave()) {
        for (uint8_t i = 0; i < numValues; i++) {
            _values[i] = values[i];
        }
    } else {
        Serial.println(F("Is Master, not updating values"));
    }
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

    if (isSlave()) {
        for (uint8_t i = 0; i < numSettings; i++) {
            _settings[i] = settings[i];
        }
    } else {
        Serial.println(F("Is Master, not updating settings"));
    }
}

void PumpSwitch::setLongOffMinutes(bool enabled) {
    if (enabled) {
        _settings[PUMP_SETTINGS_MIN_OFF_MINUTES] = PUMP_LONG_MIN_OFF_MINUTES;
    } else {
        _settings[PUMP_SETTINGS_MIN_OFF_MINUTES] = PUMP_DEFAULT_MIN_OFF_MINUTES;
    }
}

void PumpSwitch::setLongOnMinutes(bool enabled) {
    if (enabled) {
        _settings[PUMP_SETTINGS_MAX_ON_MINUTES] = PUMP_LONG_MAX_ON_MINUTES;
    } else {
        _settings[PUMP_SETTINGS_MAX_ON_MINUTES] = PUMP_DEFAULT_MAX_ON_MINUTES;
    }
}

uint8_t* PumpSwitch::getSettings() {
    return _settings;
}

uint8_t PumpSwitch::getNumSettings() {
    return PUMP_SETTINGS_TOTAL;
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
    return _calculateElapsedSeconds(_values);
}

uint32_t PumpSwitch::getElapsedMinutes() {
    return getElapsedSeconds() / 60L;
}

uint8_t PumpSwitch::getValueSeconds() {
    return _values[PUMP_VALUES_SECONDS];
}

uint8_t PumpSwitch::getValueMinutes() {
    return _values[PUMP_VALUES_MINUTES];
}

uint8_t PumpSwitch::getValueHours() {
    return _values[PUMP_VALUES_HOURS];
}

uint8_t PumpSwitch::getValueDays() {
    return _values[PUMP_VALUES_DAYS];
}

/*
 * Check how long the pump has been running and 
 * stop the pump if it exceeds the limit.
 */
void PumpSwitch::check() {
    _updateElapsedTime();

    _led.check();

    if (_debouncer.update() && _debouncer.fell()) {
        // button was pressed
        isOn() ? stop(true) : start();
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
 * or via request from a remote PumpSwitch 
 */
bool PumpSwitch::start(bool force) {
    Serial.print(F("Starting pump for "));
    Serial.print(getMaxOnMinutes());
    Serial.println(F("min"));

    if (isOn()) {
        Serial.print(F("Pump already running for "));
        Serial.print(getElapsedMinutes());
        Serial.println(F("min"));
        // don't do anything if the pump is already running
        return true;
    }

    // don't start the pump if it hasn't rested long enough
    if (getMinOffMinutes() > getElapsedMinutes()) {
        if (!_startAttemptTime || millis() - _startAttemptTime < PUMP_START_ATTEMPTS_WINDOW_SECONDS * 1000LU) {
            _startAttemptCount++;
        } else {
            _startAttemptTime = millis();
            _startAttemptCount = 1;
        }

        if (!force && PUMP_MIN_START_ATTEMPTS > _startAttemptCount) {
            Serial.print(F("Pump only off "));
            Serial.print(getElapsedMinutes());
            Serial.print(F("min (MINIMUM: "));
            Serial.print(getMinOffMinutes());
            Serial.println(F("min), NOT starting"));

            _led.error();
            return false;
        } else {
            Serial.println(F("Pump override enabled!"));
        }
    }

    // pump is starting, reset the attempt counter/time
    _startAttemptTime = 0;
    _startAttemptCount = 0;

    _led.thinking();

    // turn on the LED
    _led.on();

    // set the internal state
    _on();

    // actually start the pump!
    (*_startCallback)();

    Serial.println(F("Pump started"));
    return true;
}

/*
 * Stop the pump, triggered either by user input (button)
 * or via request from the base station.
 */
bool PumpSwitch::stop(bool force) {
    Serial.print(F("Stopping pump, cannot restart for "));
    Serial.print(getMinOffMinutes());
    Serial.println(F("min"));

    if (isOff()) {
        Serial.print(F("Pump was already stopped "));
        Serial.print(getElapsedMinutes());
        Serial.println(F("min ago"));

        // don't do anything if the pump is already stopped
        return true;
    }

    // don't stop the pump if it hasn't run long enough
    if (getMinOnMinutes() > getElapsedMinutes()) {
        if (!force) {
            Serial.print(F("Pump only on "));
            Serial.print(getElapsedMinutes());
            Serial.print(F("min (MINIMUM: "));
            Serial.print(getMinOnMinutes());
            Serial.println(F("min), NOT stopping"));

            _led.error();
            return false;
        } else {
            Serial.println(F("Pump override enabled!"));
        }
    }

    _led.thinking();

    // turn off the LED
    _led.off();

    // set the internal state
    _off();

    // actually stop the pump!
    (*_stopCallback)();

    Serial.println(F("Pump stopped"));
    return true;
}

