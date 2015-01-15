#ifndef Display_h
#define Display_h

// system
#include <Arduino.h>

// local
#include "Bargraph.h"
#include "Counter.h"
#include "LED.h"
#include "Message.h"
#include "PumpSwitch.h"
#include "TankSensors.h"

/*
 * Constants
 */

#define DISPLAY_TOTAL_VALVE_LEDS 5
#define DISPLAY_TOTAL_BARGRAPHS  3
#define DISPLAY_STATUS_INTERVAL_SECONDS 15

class Display {
    private:
        Message _message;
        Counter _counter;
        Bargraph _bar[DISPLAY_TOTAL_BARGRAPHS];
        LED _latePumpSwitchLed;
        LED _lateTankSensorsLed;
        LED _valveLed[DISPLAY_TOTAL_VALVE_LEDS];

        void _updateCounter(PumpSwitch &pumpSwitch);
        void _scrollStatus(PumpSwitch &pumpSwitch);
        void _updateBars(TankSensors &tankSensors);
        void _updateLateLEDs(PumpSwitch &pumpSwitch, bool pumpSwitchLate, TankSensors &tankSensors, bool tankSensorsLate);
        void _updateValveLEDs(TankSensors &tankSensors);

        uint32_t _lastStatusTime;

    public:
        Display(Message &message, 
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
                LED &valveLed_5);

        ~Display();

        void setup();
        void check(PumpSwitch &pumpSwitch, bool pumpSwitchLate, TankSensors &tankSensors, bool tankSensorsLate);

        void scroll(char* msg);

        void reset();
};

#endif //Display_h
