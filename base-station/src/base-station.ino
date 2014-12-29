/*
 * Receive data from the Remote Sensors and the
 * Pump Switch, display the data, and manage the
 * Pump Switch.
 *
 */

#include <XBee.h>
#include <SoftwareSerial.h>

#include "Danaides.h"

/*
 * Constants
 */

#define STATUS_LED 13
#define ERROR_LED  13

uint8_t ssRx = 12;
uint8_t ssTx = 11;
SoftwareSerial ss(ssRx, ssTx);

XBee xbee = XBee();
XBeeResponse response = XBeeResponse();

// create reusable response objects for responses we expect to handle
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();

void flashLed(int pin, int times, int wait) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(wait);
        digitalWrite(pin, LOW);

        if (i + 1 < times) {
            delay(wait);
        }
    }
}

void setup() {
    // hardware serial is being used for FTDI debugging
    Serial.begin(9600);
    Serial.println("setup()");

    pinMode(STATUS_LED, OUTPUT);
    pinMode(ERROR_LED, OUTPUT);

    // software serial is used for XBee communications
    ss.begin(9600);
    xbee.setSerial(ss);
}

void loop() {
    xbee.readPacket();

    if (xbee.getResponse().isAvailable()) {
        Serial.print("RESPONSE_API_ID=");
        Serial.print(xbee.getResponse().getApiId());
        Serial.print(" i.e. ");

        if (ZB_RX_RESPONSE == xbee.getResponse().getApiId()) {
            xbee.getResponse().getZBRxResponse(rx);

            Serial.println("ZB_RX_RESPONSE");

            if (ZB_PACKET_ACKNOWLEDGED == rx.getOption()) {
                // sender got an ACK
                flashLed(STATUS_LED, 10, 10);
            } else {
                // no ACK for sender
                flashLed(ERROR_LED, 2, 20);
            }

            Serial.print("Packet length is ");
            Serial.println(rx.getPacketLength(), DEC);

            Serial.print("Data length is ");
            Serial.println(rx.getDataLength(), DEC);

            Serial.println("Received data: ");
            for (int i = 0; i < rx.getDataLength(); i++) {
                Serial.print(i);
                Serial.print(": [");
                Serial.print(rx.getData()[i], DEC);
                Serial.println("]");
            }

        } else if (MODEM_STATUS_RESPONSE == xbee.getResponse().getApiId()) {
            // the local XBee sends this response on certain events, like assoc/disassoc
            xbee.getResponse().getModemStatusResponse(msr);

            Serial.print("MODEM_STATUS_RESPONSE=");
            Serial.print(msr.getStatus());
            Serial.print(" i.e. ");

            if (ASSOCIATED == msr.getStatus()) {
                Serial.println("ASSOCIATED");
                flashLed(STATUS_LED, 10, 10);
            } else if (DISASSOCIATED == msr.getStatus()) {
                Serial.println("DISASSOCIATED");
                flashLed(ERROR_LED, 10, 10);
            } else {
                Serial.println("OTHER");
                // something else
                flashLed(STATUS_LED, 5, 10);
            }
        } else {
            Serial.println("UNEXPECTED");

            // not something we were expecting
            flashLed(STATUS_LED, 5, 10);
        }
    } else if (xbee.getResponse().isError()) {
        Serial.print("Error reading packet. Error code: ");
        Serial.println(xbee.getResponse().getErrorCode());
    }
        
    // no delay, keep checking packets immediately forever...
}
