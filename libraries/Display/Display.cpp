// system
#include <Arduino.h>

// local
#include "Bargraph.h"
#include "Counter.h"
#include "Display.h"
#include "LED.h"
#include "Message.h"
#include "PumpSwitch.h"
#include "TankSensors.h"

/*
 * Private
 */

// for each tank, start at the top sensor (#0) and walk down,
// display each bar graph block. Any sensor that is ON will 
// set a sticky state for all lower sensors to fill their blocks
// regardless of their own state.
void Display::_updateBars(TankSensors &tankSensors) {
    if (tankSensors.ready()) {
        //XXX assumes that tankSensors.getNumTanks == DISPLAY_TOTAL_BARGRAPHS
        for (uint8_t t = 0; t < tankSensors.getNumTanks(); t++) {
            bool fill = false;
            for (uint8_t f = 0; f < tankSensors.getNumFloatsPerTank(); f++) {
                bool state = tankSensors.getFloatState(t+1, f+1);
                if (state) {
                    fill = true;
                }
                _bar[t].setBlock(state, fill, f);
            }
        }
    } 

    for (uint8_t i = 0; i < DISPLAY_TOTAL_BARGRAPHS; i++) {
        _bar[i].check();
    }
}

// calculate the remaining pump run time and display it
void Display::_updateCounter(PumpSwitch &pumpSwitch) {
    int32_t elapsedSeconds = pumpSwitch.getMaxOnMinutes() * 60UL - pumpSwitch.getElapsedSeconds();
    _counter.check(pumpSwitch.isOn(), elapsedSeconds);
}

void Display::_updateLateLEDs(PumpSwitch &pumpSwitch, bool pumpSwitchLate, TankSensors &tankSensors, bool tankSensorsLate) {
    if (pumpSwitch.ready() && !pumpSwitchLate) {
        _latePumpSwitchLed.flashing(false);
        _latePumpSwitchLed.on();
    } else if (pumpSwitchLate) {
        _latePumpSwitchLed.flashing(true);
    } else {
        _latePumpSwitchLed.off();
    }

    if (tankSensors.ready() && !tankSensorsLate) {
        _lateTankSensorsLed.flashing(false);
        _lateTankSensorsLed.on();
    } else if (tankSensorsLate) {
        _lateTankSensorsLed.flashing(true);
    } else {
        _lateTankSensorsLed.off();
    }

    _latePumpSwitchLed.check();
    _lateTankSensorsLed.check();
}

void Display::_updateValveLEDs(TankSensors &tankSensors) {
    for (uint8_t i = 0; i < DISPLAY_TOTAL_VALVE_LEDS; i++) {
        if (tankSensors.ready()) {
            if (tankSensors.getValveState(i+1)) {
                _valveLed[i].flashing(true);
            } else {
                _valveLed[i].flashing(false);
                _valveLed[i].on();
            }
        } else {
            _valveLed[i].off();
        }

        _valveLed[i].check();
    }
}

void Display::_scrollStatus(PumpSwitch &pumpSwitch) {
    if (millis() - _lastStatusTime > DISPLAY_STATUS_INTERVAL_SECONDS * 1000UL) {
        _lastStatusTime = millis();

        char state[4];
        if (pumpSwitch.isOn()) {
            snprintf(state, 4, "ON");
        } else {
            snprintf(state, 4, "OFF");
        }

        char duration_1[9];
        char duration_2[9];
        if (pumpSwitch.getValueDays()) {
            snprintf(duration_1, 9, "%u DAYS", pumpSwitch.getValueDays());
            snprintf(duration_2, 9, "%u HRS", pumpSwitch.getValueHours());
        } else if (pumpSwitch.getValueHours()) {
            snprintf(duration_1, 9, "%u HRS", pumpSwitch.getValueHours());
            snprintf(duration_2, 9, "%u MIN", pumpSwitch.getValueMinutes());
        } else {
            snprintf(duration_1, 9, "%u MIN", pumpSwitch.getValueMinutes());
            snprintf(duration_2, 9, "%u SEC", pumpSwitch.getValueSeconds());
        }

        char status[34];
        snprintf(status, 34, "PUMP STATUS %s %s %s", state, duration_1, duration_2);
        scroll(status);
    }

    _message.check();
}

/*
 * Public
 */

Display::Display(Message &message, 
                 Counter &counter, 
                 Bargraph &bar_1, 
                 Bargraph &bar_2, 
                 Bargraph &bar_3,
                 LED &latePumpSwitchLed,
                 LED &lateTankSensorsLed,
                 LED &valveLed_1,
                 LED &valveLed_2,
                 LED &valveLed_3,
                 LED &valveLed_4,
                 LED &valveLed_5) :
                 _message(message),
                 _counter(counter),
                 _latePumpSwitchLed(latePumpSwitchLed),
                 _lateTankSensorsLed(lateTankSensorsLed),
                 _lastStatusTime(0UL) {
    _bar[0] = bar_1;
    _bar[1] = bar_2;
    _bar[2] = bar_3;

    _valveLed[0] = valveLed_1;
    _valveLed[1] = valveLed_2;
    _valveLed[2] = valveLed_3;
    _valveLed[3] = valveLed_4;
    _valveLed[4] = valveLed_5;
}

Display::~Display() {
}

void Display::setup() {
    _message.setup();
    _counter.setup();

    for (uint8_t i = 0; i < DISPLAY_TOTAL_BARGRAPHS; i++) {
        _bar[i].setup();
    }

    for (uint8_t i = 0; i < DISPLAY_TOTAL_VALVE_LEDS; i++) {
        _valveLed[i].setup();
    }

    _latePumpSwitchLed.setup();
    _lateTankSensorsLed.setup();

    reset();
}

void Display::reset() {
    _message.reset();
    _counter.reset();

    for (uint8_t i = 0; i < DISPLAY_TOTAL_BARGRAPHS; i++) {
        _bar[i].reset();
    }
}

void Display::check(PumpSwitch &pumpSwitch, bool pumpSwitchLate, TankSensors &tankSensors, bool tankSensorsLate) {
    _scrollStatus(pumpSwitch);

    _updateCounter(pumpSwitch);

    _updateBars(tankSensors);

    _updateLateLEDs(pumpSwitch, pumpSwitchLate, tankSensors, tankSensorsLate);

    _updateValveLEDs(tankSensors);
} 

void Display::scroll(char* msg) {
    _message.setMessage(msg);
    _lastStatusTime = millis();
}


