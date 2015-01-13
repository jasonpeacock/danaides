#ifndef WAN_h
#define WAN_h

// third-party
#include "XBee.h"

// local
#include "LED.h"
#include "Data.h"

/*
 * Constants
 */

// XBee radio settings
#define XBEE_FAMILY_ADDRESS        0x0013A200UL
#define XBEE_BASE_STATION_ADDRESS  0x40C59926UL
#define XBEE_REMOTE_SENSOR_ADDRESS 0x40C59899UL
#define XBEE_PUMP_SWITCH_ADDRESS   0x40C31683UL

#define XBEE_SLEEP_DELAY_MILLIS 50UL

class WAN {
    private:
        LED _statusLed;

        XBee _xbee;
        ZBRxResponse _zbRx;
        ZBTxStatusResponse _zbTxStatus;

        void _init(Stream &serial);

        // With Sleep Mode = 1 (Pin), setting DTR
        // high will sleep the XBee.
        uint8_t _dtrPin;

        // CTS is used to determine when XBee is ready to transmit
        // after waking from sleep.
        uint8_t _ctsPin;

        bool _sleepEnabled;

        // this is managed automatically, doesn't need to be public
        void _sleep();
        void _wake();

    public:
        WAN(Stream &serial);
        WAN(Stream &serial, LED statusLed);
        ~WAN();

        void setup();
        void check();

        // will sleep the XBee when not receiving/transmitting
        void enableSleep(uint8_t dtrPin, uint8_t ctsPin);
        void disableSleep();

        bool receive(Data &data);
        bool transmit(Data *data);

        uint32_t getBaseStationAddress();
        uint32_t getRemoteSensorAddress();
        uint32_t getPumpSwitchAddress();
        bool isBaseStationAddress(uint32_t address);
        bool isRemoteSensorAddress(uint32_t address);
        bool isPumpSwitchAddress(uint32_t address);

};

#endif //WAN_h
