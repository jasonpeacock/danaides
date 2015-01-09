// system
#include <Arduino.h>

// local
#include "Data.h"
#include "TankSensors.h"

/*
 * Private
 */

bool TankSensors::_getSensorState(uint8_t sensorIndex) {
    if (getNumSensors() - 1 < sensorIndex) {
        // anything unkown is OFF
        Serial.print(F("Unknown sensor index: "));
        Serial.println(sensorIndex);
        return false;
    }

    if (TANK_1_INVERTED_FLOAT == sensorIndex) {
        return !_sensors[sensorIndex];
    }

    return _sensors[sensorIndex];
}

/*
 * Public
 */

TankSensors::TankSensors() {
}

TankSensors::~TankSensors() {
}

void TankSensors::setup() {
    // nothing to do
}

void TankSensors::update(Data &data) {
    for (uint8_t i = 0; i < data.getSize(); i++) {
        _sensors[i] = data.getData()[i];
    }
}

bool TankSensors::getValveState(uint8_t valveNumber) {
    if (SENSOR_TOTAL_VALVES < valveNumber || 0 == valveNumber) {
        // anything unkown is OFF
        Serial.print(F("Unknown valve number: "));
        Serial.println(valveNumber);
        return false;
    }

    return _getSensorState(VALVE_POSITION_OFFSET + valveNumber);
}

bool TankSensors::getFloatState(uint8_t tankNumber, uint8_t floatNumber) {
    if (SENSOR_TOTAL_FLOATS_PER_TANK < floatNumber || 0 == floatNumber) {
        // anything unkown is OFF
        Serial.print(F("Unknown float number: "));
        Serial.println(floatNumber);
        return false;
    }

    if (SENSOR_TOTAL_TANKS < tankNumber || 0 == tankNumber) {
        // anything unkown is OFF
        Serial.print(F("Unknown tank number: "));
        Serial.println(tankNumber);
        return false;
    }

    if (1 == tankNumber) {
        if (TANK_1_FLOAT_1 == TANK_1_FLOAT_OFFSET + floatNumber) {
            // the 1st float sensor on the 1st tank is a compound sensor,
            // both sensors must be OFF at the same time, while either sensor
            // can be ON. This is mean to provide insurance against over-filling
            // the tank if one sensor goes bad.
            return _getSensorState(TANK_1_FLOAT_1) || _getSensorState(TANK_1_INVERTED_FLOAT);
        }

        return _getSensorState(TANK_1_FLOAT_OFFSET + floatNumber);
    } else if (2 == tankNumber) {
        return _getSensorState(TANK_2_FLOAT_OFFSET + floatNumber);
    } else if (3 == tankNumber) {
        return _getSensorState(TANK_3_FLOAT_OFFSET + floatNumber);
    }
}

bool TankSensors::getTankState(uint8_t tankNumber) {
    // the 1st float in each tank is the top float,
    // which detects if it's full or not.
    return getFloatState(tankNumber, 1);
}

void TankSensors::check() {
}

uint8_t TankSensors::getNumSensors() {
    return SENSOR_TOTAL_INPUTS;
}
