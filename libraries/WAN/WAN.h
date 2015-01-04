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
#define XBEE_FAMILY_ADDRESS        0x0013A200
#define XBEE_BASE_STATION_ADDRESS  0x40C59926
#define XBEE_REMOTE_SENSOR_ADDRESS 0x40C59899
#define XBEE_PUMP_SWITCH_ADDRESS   0x40C31683

// how long to wait for a status response after sending a message
#define XBEE_STATUS_WAIT_MS 500

class WAN {
    private:
        LED _statusLed;
        XBee _xbee;
        ZBRxResponse _zbRx;

        void _init(Stream &serial);

    public:
        WAN(Stream &serial);
        WAN(Stream &serial, LED statusLed);
        ~WAN();

        void setup();

        bool receive(Data &data);
        bool transmit(Data data);

};

#endif //WAN_h
