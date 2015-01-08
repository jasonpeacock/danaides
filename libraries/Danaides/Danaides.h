#ifndef Danaides_h
#define Danaides_h

#include <Arduino.h>

void freeRam () {
    extern int __heap_start, *__brkval;
    int v;
    Serial.print(F("Free RAM (B): "));
    Serial.println((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}

/*
 * Remote Sensor Values
 */

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
uint8_t sensorValues[SENSOR_TOTAL_INPUTS];

#endif //Danaides_h
