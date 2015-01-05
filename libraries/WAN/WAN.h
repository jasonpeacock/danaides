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

class WAN {
    private:
        LED _statusLed;

        XBee _xbee;
        ZBRxResponse _zbRx;
        ZBTxStatusResponse _zbTxStatus;

        void _init(Stream &serial);

    public:
        WAN(Stream &serial);
        WAN(Stream &serial, LED statusLed);
        ~WAN();

        void setup();

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
