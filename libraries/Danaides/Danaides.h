#ifndef Danaides_h
#define Danaides_h

// system
#include <Arduino.h>

//XXX update to correct values! (10min/30min?)

// how often to check sensors
// NOTE: if all sensor values match previous values
//       then NO update will be transmitted.
#define REMOTE_SENSOR_CHECK_INTERVAL_SECONDS 10UL
// how often to transmit values, regardless of previous sensor values
#define REMOTE_SENSOR_FORCE_TRANSMIT_INTERVAL_SECONDS 30UL

void freeRam () {
    extern int __heap_start, *__brkval;
    int v;
    Serial.print(F("Free RAM (B): "));
    Serial.println((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}

#endif //Danaides_h
