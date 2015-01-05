// system
#include <inttypes.h>

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
                           _zbTxStatus(ZBTxStatusResponse()),
                           _statusLed(LED(0)) {
    _init(serial);
}

WAN::WAN(Stream &serial, LED statusLed) : _xbee(XBee()), 
                                          _zbRx(ZBRxResponse()), 
                                          _zbTxStatus(ZBTxStatusResponse()),
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

            data.set(_zbRx.getRemoteAddress64().getLsb(), _zbRx.getData(), _zbRx.getDataLength());

            // data has been updated
            received = true;

        } else if (ZB_TX_STATUS_RESPONSE == _xbee.getResponse().getApiId()) {
            _xbee.getResponse().getZBTxStatusResponse(_zbTxStatus);

            if (SUCCESS == _zbTxStatus.getDeliveryStatus()) {
                dbg("Delivery Success!");
            } else {
                dbg("Delivery Failure :(");
            }
        } else {
            dbg("UNEXPECTED RESPONSE: %d", _xbee.getResponse().getApiId());
        }
    } else if (_xbee.getResponse().isError()) {
        dbg("Error reading packet. Error code: %d", _xbee.getResponse().getErrorCode());
    }

    return received;
}

bool WAN::transmit(Data *data) {
    XBeeAddress64 addr64 = XBeeAddress64(XBEE_FAMILY_ADDRESS, data->getAddress());
    ZBTxRequest zbTx = ZBTxRequest(addr64, data->getData(), data->getSize());
    
    dbg("Sending %u values to 0x%lX", data->getSize(), data->getAddress());

    _xbee.send(zbTx);

    return true;
}

uint32_t WAN::getBaseStationAddress() {
    return XBEE_BASE_STATION_ADDRESS;
}

uint32_t WAN::getRemoteSensorAddress() {
    return XBEE_REMOTE_SENSOR_ADDRESS;
}

uint32_t WAN::getPumpSwitchAddress() {
    return XBEE_PUMP_SWITCH_ADDRESS;
}

bool WAN::isBaseStationAddress(uint32_t address) {
    return XBEE_BASE_STATION_ADDRESS == address;
}

bool WAN::isRemoteSensorAddress(uint32_t address) {
    return XBEE_REMOTE_SENSOR_ADDRESS == address;
}

bool WAN::isPumpSwitchAddress(uint32_t address) {
    return XBEE_PUMP_SWITCH_ADDRESS == address;
}
