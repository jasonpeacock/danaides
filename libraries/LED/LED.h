#ifndef LED_h
#define LED_h

// system
#include <Arduino.h>

/*
 * Constants
 */

#define LED_HEARTBEAT_INTERVAL_SECONDS 5UL
#define LED_FLASHING_INTERVAL 250UL

class LED {
    private:
        uint8_t _pin;

        bool _currLedState; // actual LED state
        bool _on;           // desired LED state

        uint8_t  _maxFlashCount;
        uint8_t  _currFlashCount;
        uint32_t _flashDelayMs;
        uint32_t _lastFlashTime;
        bool     _flashing;

        uint32_t _lastHeartbeatTime;
        void _resetHeartbeat();

        void _checkFlash();
        void _checkFlashing();

    public:
        LED();
        LED(uint8_t pin);
        ~LED();

        void setup();
        void check();

        bool enabled();
        void on();
        void off();

        void flash(uint8_t times, uint32_t wait);
        bool completedFlashing();

        void flashing(bool enable);
        void thinking();
        void success();
        void error();
        void heartbeat();
};

#endif //LED_h
