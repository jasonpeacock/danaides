#include <Arduino.h>

#include "Bounce2.h"
#include "Dbg.h"

#include "LED.h"
#include "PumpSwitch.h"

/*
 * Private
 */

uint32_t PumpSwitch::_msToMinutes(uint32_t ms) {
    return (ms / 1000) / 60;
}

void PumpSwitch::_resetPumpValues(bool running) {
    _time = millis();
    _pumpValues[PUMP_STATE]   = running;
    _pumpValues[PUMP_DAYS]    = 0;
    _pumpValues[PUMP_HOURS]   = 0;
    _pumpValues[PUMP_MINUTES] = 0;
    _pumpValues[PUMP_SECONDS] = 0;
}

void PumpSwitch::_on() {
    _resetPumpValues(true);
}

void PumpSwitch::_off() {
    _resetPumpValues(false);
}

/*
 * Calculate the elapsed time since the pump started/stopped
 * as Days, Hours, Minutes, Seconds and update the pumpValues.
 *
 * The Days & Hours are calculated for the pumpValues because
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

    _pumpValues[PUMP_DAYS]    = elapsedDays;
    _pumpValues[PUMP_HOURS]   = remainderHours;
    _pumpValues[PUMP_MINUTES] = remainderMinutes;
    _pumpValues[PUMP_SECONDS] = remainderSeconds;
}

/*
 * Public
 */

PumpSwitch::PumpSwitch(int buttonPin, int ledPin, void (*startCallback)(), void (*stopCallback)()) {
    Debug.begin();

    _debouncer = Bounce(); 
    _buttonPin = buttonPin;

    _led = LED(ledPin);

    _startCallback = startCallback;
    _stopCallback = stopCallback;

    _time = 0;

    _lastStartAttemptTime = 0;
    _startAttemptCount = 0;

    _maxOnMinutes  = PUMP_DEFAULT_MAX_ON_MINUTES;
    _minOffMinutes = PUMP_DEFAULT_MIN_OFF_MINUTES;

    // start with the pump off
    _off();
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
    return _pumpValues[PUMP_STATE] ? true : false;
}

bool PumpSwitch::isOff() {
    return !isOn();
}

uint8_t* PumpSwitch::getPumpValues() {
    return _pumpValues;
}

uint8_t PumpSwitch::getNumPumpValues() {
    return PUMP_TOTAL_VALUES;
}

int PumpSwitch::getMaxOnMinutes() {
    return _maxOnMinutes;
}

int PumpSwitch::getMinOffMinutes() {
    return _minOffMinutes;
}

uint32_t PumpSwitch::getElapsedSeconds() {
    uint32_t seconds = 0UL;
    seconds += _pumpValues[PUMP_SECONDS];
    seconds += _pumpValues[PUMP_MINUTES] * 60UL;
    seconds +=   _pumpValues[PUMP_HOURS] * 60UL * 60UL;
    seconds +=    _pumpValues[PUMP_DAYS] * 60UL * 60UL * 24UL;

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
        if (_maxOnMinutes <= onMinutes) {
            stop();
        } else {
            // do nothing
        }
    }

    // regardless of pump state, always blink/flash
    _led.heartbeat();
}

/*
 * Start the pump, triggered either by user input (button)
 * or via request from the base station.
 */
void PumpSwitch::start() {
    dbg("Starting pump...");

    if (isOn()) {
        dbg("Pump is already running for %lumin", _msToMinutes(millis() - _time));
        // don't do anything if the pump is already running
        return;
    }

    // don't start the pump if it hasn't rested long enough
    // TODO make this configurable with switches on base station, allow base station
    // to transmit overrides to the default values...
    uint32_t offMinutes = _msToMinutes(millis() - _time);
    if (_time && _minOffMinutes > offMinutes) {
        if (!_lastStartAttemptTime || millis() - _lastStartAttemptTime < PUMP_START_ATTEMPTS_WINDOW_SECONDS * 1000LU) {
            _startAttemptCount++;
        } else {
            _lastStartAttemptTime = millis();
            _startAttemptCount = 1;
        }

        if (PUMP_MIN_START_ATTEMPTS > _startAttemptCount) {
            dbg("Pump has only been off %lumin (MINIMUM: %dmin), NOT starting", offMinutes, _minOffMinutes);
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
    dbg("Stopping pump");

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

