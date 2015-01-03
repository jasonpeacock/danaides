#ifndef LED_h
#define LED_h

/*
 * Constants
 */

#define LED_HEARTBEAT_INTERVAL_SECONDS 5

class LED {
    private:
        int _pin;
        uint32_t _lastHeartbeatTime;
        bool _enabled;

        void _init(int pin);

    public:
        LED();
        LED(int pin);
        ~LED();

        // to be called during setup() in main Arduino sketch,
        // will setup the pins properly and anything else.
        void setup();

        void on();
        void off();

        void flash(int times, int wait);
        void thinking();
        void error();
        void heartbeat();
};

#endif //LED_h
