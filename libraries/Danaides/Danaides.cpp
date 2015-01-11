#include "Danaides.h"

void freeRam () {
    extern int __heap_start, *__brkval;
    int v;
    Serial.print(F("Free RAM (B): "));
    Serial.println((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}


