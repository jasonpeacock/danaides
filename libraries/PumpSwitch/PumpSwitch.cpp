// third-party
#include "Dbg.h"

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
    _settings[PUMP_SETTINGS_MAX_ON_MINUTES]  = PUMP_DEFAULT_MAX_ON_MINUTES;
    _settings[PUMP_SETTINGS_MIN_OFF_MINUTES] = PUMP_DEFAULT_MIN_OFF_MINUTES;
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
    uint32_t elapsedSeconds = (uint32_t)((millis() - _time) / 1000LU);
    uint32_t elapsedMinutes = (uint32_t)(elapsedSeconds / 60);
    uint32_t elapsedHours   = (uint32_t)(elapsedMinutes / 60);
    uint8_t  elapsedDays    = (uint32_t)(elapsedHours / 24);

    uint32_t remainderHours   = elapsedHours % 24;
    uint32_t remainderMinutes = elapsedMinutes % 60;
    uint32_t remainderSeconds = elapsedSeconds % 60;

    _values[PUMP_VALUES_DAYS]    = elapsedDays;
    _values[PUMP_VALUES_HOURS]   = remainderHours;
    _values[PUMP_VALUES_MINUTES] = remainderMinutes;
    _values[PUMP_VALUES_SECONDS] = remainderSeconds;
}

/*
 * Public
 */

PumpSwitch::PumpSwitch(uint8_t buttonPin, uint8_t ledPin, void (*startCallback)(), void (*stopCallback)()) {
    Debug.begin();

    _debouncer = Bounce(); 
    _buttonPin = buttonPin;

    _led = LED(ledPin);

    _startCallback = startCallback;
    _stopCallback = stopCallback;

    _time = 0;

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
        dbg("[ERROR] numValues (%u) != PUMP_VALUES_TOTAL (%u)", numValues, PUMP_VALUES_TOTAL);
        return;
    }

    if (_values[PUMP_VALUES_STATE] != values[PUMP_VALUES_STATE]) {
        dbg("New values have a different pump state, updating Pump Switch");
        // the updated values reflect a different pump state,
        // update (force) our Pump Switch to match
        values[PUMP_VALUES_STATE] ? start(true) : stop();
    }

    // and now copy all the values to match (especially the elapsed times)
    for (uint8_t i = 0; i < numValues; i++) {
        _values[i] = values[i];
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
        dbg("[ERROR] numSettings (%u) != PUMP_SETTINGS_TOTAL (%u)", numSettings, PUMP_SETTINGS_TOTAL);
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

uint8_t PumpSwitch::getMaxOnMinutes() {
    return _settings[PUMP_SETTINGS_MAX_ON_MINUTES];
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
        uint32_t onMinutes = _msToMinutes(millis() - _time);
        if (getMaxOnMinutes() <= onMinutes) {
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
    dbg("Starting pump for %u minutes", getMaxOnMinutes());

    if (isOn()) {
        dbg("Pump is already running for %lumin", _msToMinutes(millis() - _time));
        // don't do anything if the pump is already running
        return;
    }

    // don't start the pump if it hasn't rested long enough
    uint32_t offMinutes = _msToMinutes(millis() - _time);
    if (_time && getMinOffMinutes() > offMinutes) {
        if (!_lastStartAttemptTime || millis() - _lastStartAttemptTime < PUMP_START_ATTEMPTS_WINDOW_SECONDS * 1000LU) {
            _startAttemptCount++;
        } else {
            _lastStartAttemptTime = millis();
            _startAttemptCount = 1;
        }

        if (!force && PUMP_MIN_START_ATTEMPTS > _startAttemptCount) {
            dbg("Pump has only been off %lumin (MINIMUM: %umin), NOT starting", offMinutes, getMinOffMinutes());
            _led.error();
            return;
        } else {
            dbg("Pump override enabled!");

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

    dbg("Pump started");
}

/*
 * Stop the pump, triggered either by user input (button)
 * or via request from the base station.
 */
void PumpSwitch::stop() {
    dbg("Stopping pump, cannot restart for %u minutes", getMinOffMinutes());

    if (isOff()) {
        dbg("Pump was already stopped %lumin ago", _msToMinutes(millis() - _time));
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

    dbg("Pump stopped");
}

