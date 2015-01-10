#ifndef TankSensors_h
#define TankSensors_h

// local
#include "Data.h"

/*
 * Constants
 */

// Remote Sensor Values
#define TANK_1_FLOAT_0  0
#define TANK_1_FLOAT_1  1
#define TANK_1_FLOAT_2  2
#define TANK_1_FLOAT_3  3
#define TANK_1_FLOAT_4  4
#define TANK_1_FLOAT_5  5
#define TANK_1_FLOAT_6  6

// array offset such that Tank #1, Float #1 => index 1
#define TANK_1_FLOAT_OFFSET 0

// Tank 1 has *two* float switches at the top, with
// one of them inverted for redundancy.
#define TANK_1_INVERTED_FLOAT TANK_1_FLOAT_0
 
#define TANK_2_FLOAT_1  7
#define TANK_2_FLOAT_2  8
#define TANK_2_FLOAT_3  9
#define TANK_2_FLOAT_4 10
#define TANK_2_FLOAT_5 11
#define TANK_2_FLOAT_6 12

// array offset such that Tank #1, Float #1 => index 1
#define TANK_2_FLOAT_OFFSET 6

#define TANK_3_FLOAT_1 13
#define TANK_3_FLOAT_2 14
#define TANK_3_FLOAT_3 15
#define TANK_3_FLOAT_4 16
#define TANK_3_FLOAT_5 17
#define TANK_3_FLOAT_6 18

// array offset such that Tank #1, Float #1 => index 1
#define TANK_3_FLOAT_OFFSET 12

#define VALVE_POSITION_1 19
#define VALVE_POSITION_2 20
#define VALVE_POSITION_3 21
#define VALVE_POSITION_4 22
#define VALVE_POSITION_5 23

// array offset such that Valve #1 => index 19
#define VALVE_POSITION_OFFSET 18

#define SENSOR_TOTAL_VALVES 5
#define SENSOR_TOTAL_TANKS  3
#define SENSOR_TOTAL_FLOATS_PER_TANK 6

//XXX remove the last shift register, we don't
//need all these extra inputs?
#define UNUSED_INPUT_1   24
#define UNUSED_INPUT_2   25
#define UNUSED_INPUT_3   26
#define UNUSED_INPUT_4   27
#define UNUSED_INPUT_5   28
#define UNUSED_INPUT_6   29
#define UNUSED_INPUT_7   30 
#define UNUSED_INPUT_8   31 

// how many sensors are available
// (not how many are actually used)
#define SENSOR_TOTAL_INPUTS 32

class TankSensors {
    private:
        uint8_t _sensors[SENSOR_TOTAL_INPUTS];
 
    public:
        TankSensors();
        ~TankSensors();

        // update the raw sensor values
        bool update(Data &data);

        // return the raw sensor values
        uint8_t* getSensorValues();
        uint8_t  getNumSensors();

        // convert the raw sensor values to
        // their states, including any necessary
        // transforms of the values...
        bool getSensorState(uint8_t sensorNumber);
        bool getValveState(uint8_t valveNumber);
        bool getFloatState(uint8_t tankNumber, uint8_t floatNumber);
        bool getTankState(uint8_t tankNumber);

};

#endif //TankSensors_h
