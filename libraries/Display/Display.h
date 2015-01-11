#ifndef Display_h
#define Display_h

// system
#include <Arduino.h>

// local
#include "Bargraph.h"
#include "Counter.h"
#include "Message.h"
#include "PumpSwitch.h"
#include "TankSensors.h"

/*
 * Constants
 */

#define DISPLAY_TOTAL_BARGRAPHS 3
#define DISPLAY_STATUS_INTERVAL_SECONDS 15

class Display {
    private:
        Message _message;
        Counter _counter;
        Bargraph _bar[DISPLAY_TOTAL_BARGRAPHS];

        void _updateCounter(PumpSwitch &pumpSwitch);
        void _scrollStatus(PumpSwitch &pumpSwitch);
        void _updateBars(TankSensors &tankSensors);

        uint32_t _lastStatusTime;

    public:
        Display(Message &message, Counter &counter, Bargraph &bar_1, Bargraph &bar_2, Bargraph &bar_3);
        ~Display();

        void setup();
        void check(PumpSwitch &pumpSwitch, TankSensors tankSensors);

        void scroll(char* msg);

        void reset();
};

#endif //Display_h
