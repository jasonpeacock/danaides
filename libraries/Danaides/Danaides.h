#ifndef Danaides_h
#define Danaides_h

#include <Arduino.h>

void freeRam () {
    extern int __heap_start, *__brkval;
    int v;
    Serial.print(F("Free RAM (B): "));
    Serial.println((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}

#endif //Danaides_h
