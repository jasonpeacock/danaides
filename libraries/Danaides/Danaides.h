#ifndef Danaides_h
#define Danaides_h

#include <inttypes.h>

/*
 * XBee radio settings
 */
#define XBEE_FAMILY_ADDRESS        0x0013A200
#define XBEE_BASE_STATION_ADDRESS  0x40C59926
#define XBEE_REMOTE_SENSOR_ADDRESS 0x40C59899
#define XBEE_PUMP_SWITCH_ADDRESS   0x40C31683

// how long to wait for a status response after sending a message
#define XBEE_STATUS_WAIT_MS 500

/*
 * Remote Sensor Values
 */

// outgoing
#define TANK_1_FLOAT_0  0
#define TANK_1_FLOAT_1  1
#define TANK_1_FLOAT_2  2
#define TANK_1_FLOAT_3  3
#define TANK_1_FLOAT_4  4
#define TANK_1_FLOAT_5  5
#define TANK_1_FLOAT_6  6

// Tank 1 has *two* float switches at the top, with
// one of them inverted for redundancy.
#define TANK_1_INVERTED_FLOAT TANK_1_FLOAT_0
 
#define TANK_2_FLOAT_1  7
#define TANK_2_FLOAT_2  8
#define TANK_2_FLOAT_3  9
#define TANK_2_FLOAT_4 10
#define TANK_2_FLOAT_5 11
#define TANK_2_FLOAT_6 12

#define TANK_3_FLOAT_1 13
#define TANK_3_FLOAT_2 14
#define TANK_3_FLOAT_3 15
#define TANK_3_FLOAT_4 16
#define TANK_3_FLOAT_5 17
#define TANK_3_FLOAT_6 18

#define VALVE_POSITION_1 19
#define VALVE_POSITION_2 20
#define VALVE_POSITION_3 21
#define VALVE_POSITION_4 22
#define VALVE_POSITION_5 23

#define FLOW_OUT_BACK    24
#define FLOW_OUT_FRONT   25
#define FLOW_IN          26

#define UNUSED_INPUT_1   27
#define UNUSED_INPUT_2   28
#define UNUSED_INPUT_3   29
#define UNUSED_INPUT_4   30 
#define UNUSED_INPUT_5   31 

// how many sensors are available
// (not how many are actually used)
#define SENSOR_TOTAL_INPUTS 32
uint8_t sensorValues[SENSOR_TOTAL_INPUTS];

/*
 * Pump Switch Values
 */

// pump switch incoming data
#define OVERRIDE_MAX_PUMP_ON_MINUTES  0
#define OVERRIDE_MIN_PUMP_OFF_MINUTES 1

#define PUMP_TOTAL_SETTINGS 2
uint8_t pumpSettings[PUMP_TOTAL_SETTINGS];

#endif //Danaides_h
