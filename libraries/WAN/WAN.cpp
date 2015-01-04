//system
//#include <Arduino.h>

// third-party
#include "Dbg.h"

// local
#include "WAN.h"

/*
 * Private
 */

void WAN::_init(Stream &serial) {
    _xbee.setSerial(serial);
}

/*
 * Public
 */

WAN::WAN(Stream &serial) : _xbee(XBee()), 
                           _zbRx(ZBRxResponse()), 
                           _statusLed(LED(0)) {
    _init(serial);
}

WAN::WAN(Stream &serial, LED statusLed) : _xbee(XBee()), 
                                          _zbRx(ZBRxResponse()), 
                                          _statusLed(statusLed) {
    _init(serial);
}

WAN::~WAN() {
}

void WAN::setup() {
    Debug.begin();
    _statusLed.setup();
}

bool WAN::receive(Data &data) {
    _xbee.readPacket();

    bool received = false;

    if (_xbee.getResponse().isAvailable()) {
        if (ZB_RX_RESPONSE == _xbee.getResponse().getApiId()) {
            _xbee.getResponse().getZBRxResponse(_zbRx);

            dbg("ZB_RX_RESPONSE");

            if (ZB_PACKET_ACKNOWLEDGED == _zbRx.getOption()) {
                // sender got an ACK
                _statusLed.flash(10, 10);
            } else {
                // no ACK for sender
                _statusLed.flash(2, 20);
            }

            data.set(_zbRx.getRemoteAddress16(), _zbRx.getData(), _zbRx.getDataLength());

            dbg("Received data:");
            for (int i = 0; i < _zbRx.getDataLength(); i++) {
                dbg("%d:\t[%d]", i, _zbRx.getData()[i]);
            }

            received = true;

        } else {
            dbg("UNEXPECTED");

            // not something we were expecting
            _statusLed.flash(5, 10);
        }
    } else if (_xbee.getResponse().isError()) {
        dbg("Error reading packet. Error code: %d", _xbee.getResponse().getErrorCode());
    }

    return received;
}

bool WAN::transmit(Data data) {
    return true;
}

