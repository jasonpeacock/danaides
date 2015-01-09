// system
#include <Arduino.h>

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
                           _statusLed(LED(0)),
                           _dtrPin(0),
                           _ctsPin(0),
                           _sleepEnabled(false) {
    _init(serial);
}

WAN::WAN(Stream &serial, LED statusLed) : _xbee(XBee()), 
                                          _zbRx(ZBRxResponse()), 
                                          _zbTxStatus(ZBTxStatusResponse()),
                                          _statusLed(statusLed),
                                          _dtrPin(0),
                                          _ctsPin(0),
                                          _sleepEnabled(false) {
    _init(serial);
}

WAN::~WAN() {
}

void WAN::setup() {
    _statusLed.setup();
}

void WAN::enableSleep(uint8_t dtrPin, uint8_t ctsPin) {
    _dtrPin = dtrPin;
    _ctsPin = ctsPin;

    pinMode(_dtrPin, OUTPUT);
    digitalWrite(_dtrPin, LOW);

    pinMode(_ctsPin, INPUT);

    _sleepEnabled = true;
    Serial.println(F("Sleep is enabled"));

    // start sleeping until needed
    _sleep();
}

void WAN::disableSleep() {
    // wake the XBee before disabling our ability to do so...
    _wake();
    _sleepEnabled = false;
    Serial.println(F("Sleep is disabled"));
}

// XXX investigate using SLEEP_PIN as INPUT instead, less power?
// http://www.fiz-ix.com/2012/11/low-power-xbee-sleep-mode-with-arduino-and-pin-hibernation/
void WAN::_sleep() {
    if (_sleepEnabled) {
        Serial.println(F("Sleeping XBee"));

        //XXX wait a bit first, was sometimes sending bad
        // data (partial packets?) due to sleeping too
        // quickly. This delay (500ms) appears to have fixed it,
        // need to test more to see what delay can be decreased
        // to...
        delay(500);

        digitalWrite(_dtrPin, HIGH);
    }
}

void WAN::_wake() {
    if (_sleepEnabled) {
        Serial.println(F("Waking XBee..."));
        digitalWrite(_dtrPin, LOW);

        // empirically, this usually takes 15-17ms
        // after waking from sleep
        uint32_t start = millis();
        while (LOW != digitalRead(_ctsPin)) {
            // delay until CTS_PIN goes low
            delay(1);
        }
        Serial.print(F("XBee wake latency (ms): "));
        Serial.println(millis() - start);
    }
}

bool WAN::receive(Data &data) {
    _wake();

    _xbee.readPacket();

    bool received = false;
    if (_xbee.getResponse().isAvailable()) {
        if (ZB_RX_RESPONSE == _xbee.getResponse().getApiId()) {
            _xbee.getResponse().getZBRxResponse(_zbRx);

            data.set(_zbRx.getRemoteAddress64().getLsb(), _zbRx.getData(), _zbRx.getDataLength());

            received = true;

        } else if (ZB_TX_STATUS_RESPONSE == _xbee.getResponse().getApiId()) {
            _xbee.getResponse().getZBTxStatusResponse(_zbTxStatus);

            if (SUCCESS == _zbTxStatus.getDeliveryStatus()) {
                Serial.println(F("Delivery Success!"));
            } else {
                Serial.println(F("Delivery Failure :("));
            }
        } else {
            Serial.print(F("UNEXPECTED RESPONSE: "));
            Serial.println(_xbee.getResponse().getApiId());
        }
    } else if (_xbee.getResponse().isError()) {
        Serial.print(F("Error reading packet. Error code: "));
        Serial.println(_xbee.getResponse().getErrorCode());
    }

    _sleep();

    return received;
}

bool WAN::transmit(Data *data) {
    XBeeAddress64 addr64 = XBeeAddress64(XBEE_FAMILY_ADDRESS, data->getAddress());
    ZBTxRequest zbTx = ZBTxRequest(addr64, data->getData(), data->getSize());
    
    _wake();

    _xbee.send(zbTx);

    _sleep();

    // fire, but don't forget - the next receive() call will handle the txResponse
    // and log it..

    // TODO how to confirm success???
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
