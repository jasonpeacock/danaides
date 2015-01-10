#ifndef LED_h
#define LED_h

// system
#include <Arduino.h>

/*
 * Constants
 */

#define LED_HEARTBEAT_INTERVAL_SECONDS 5

class LED {
    private:
        uint8_t _pin;
        uint32_t _lastHeartbeatTime;
        bool _on;

        void _init(uint8_t pin);
        void _resetHeartbeat();

    public:
        LED();
        LED(uint8_t pin);
        ~LED();

        void setup();
        void check();

        bool enabled();
        void on();
        void off();

        void flash(uint8_t times, uint16_t wait);
        void thinking();
        void error();
        void heartbeat();
};

#endif //LED_h
