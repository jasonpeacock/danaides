/*
 * Control the water pump, switching it ON & OFF
 * as requested.
 *
 * Also, manage maximum run time and duty cycle
 * for the pump in case the Base Station fails
 * to stop the pump, or attempts to restart it
 * too often.
 * 
 */

#include <SoftwareSerial.h>

#include "Adafruit_LEDBackpack.h"
#include "Dbg.h"
#include "XBee.h"

#include "Danaides.h"
#include "PumpSwitch.h"
#include "LED.h"

/*
 * Constants
 */

// how often to transmit values
#define TRANSMIT_INTERVAL_SECONDS 60

// enable debug output
// 1 == ON
// 0 == OFF
#define DEBUG_ENABLED 1

/*
 * Pins
 */

#define BUTTON_PIN     3           // Momentary switch
#define BUTTON_LED     4           // Momentary switch LED (blue)
#define PUMP_RELAY_PIN 5           // Pump Relay Switch
#define UNUSED_PIN_6   6           // unused
#define NO_PIN_7       7           // no pin 7 on Trinket Pro boards
#define UNUSED_PIN_8   8           // unused
#define UNUSED_PIN_9   9           // unused
#define UNUSED_PIN_10 10           // unused
#define UNUSED_PIN_11 11           // unused
#define UNUSED_PIN_12 12           // unused
#define STATUS_LED    13           // XBee Status LED
#define UNUSED_PIN_14 14           // unused    (Analog 0)
#define UNUSED_PIN_15 15           // unused    (Analog 1)
#define SS_TX_PIN     16           // XBee RX   (Analog 2)
#define SS_RX_PIN     17           // XBee TX   (Analog 3)
#define I2C_DATA_PIN  18           // I2C Data  (Analog 4)
#define I2C_CLOCK_PIN 19           // I2C Clock (Analog 5)

/*
 * Unused pins can drain power, set them to INPUT
 * w/internal PULLUP enabled to prevent power drain.
 */
void setupUnusedPins() {
    pinMode(UNUSED_PIN_6,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_8,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_9,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_10, INPUT_PULLUP);
    pinMode(UNUSED_PIN_11, INPUT_PULLUP);
    pinMode(UNUSED_PIN_12, INPUT_PULLUP);

    pinMode(UNUSED_PIN_14, INPUT_PULLUP);
    pinMode(UNUSED_PIN_15, INPUT_PULLUP);
}

/*
 * XBee
 */ 
XBee xbee = XBee();

XBeeAddress64 addr64 = XBeeAddress64(XBEE_FAMILY_ADDRESS, XBEE_BASE_STATION_ADDRESS);

ZBTxStatusResponse txStatus = ZBTxStatusResponse();

// XBee will use SoftwareSerial for communications, reserve the HardwareSerial
// for debugging w/FTDI interface.
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

uint32_t lastTransmitTime = 0;

LED statusLed = LED(STATUS_LED);

void setupXBee() {
    dbg("Setting up XBee...");

    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);
    xbee.setSerial(ss);

    statusLed.setup();

    dbg("Completed XBee setup");
}

/*
 * Adafruit 7-segment LED Numeric display
 */
Adafruit_7segment counter = Adafruit_7segment();

void setupCounter() {
    counter.begin(0x70); // default address for first/single display
    counter.clear();
    counter.writeDisplay();
}

/*
 * Pump Relay
 */
void setupRelay() {
    pinMode(PUMP_RELAY_PIN, OUTPUT);
    digitalWrite(PUMP_RELAY_PIN, LOW);
}

void enableRelay() {
    digitalWrite(PUMP_RELAY_PIN, HIGH);
}

void disableRelay() {
    digitalWrite(PUMP_RELAY_PIN, LOW);
}

/*
 * Pump Switch (MAIN)
 */
PumpSwitch pumpSwitch = PumpSwitch(BUTTON_PIN, BUTTON_LED, enableRelay, disableRelay);

/*
 * Calculate the remaining time since the pump started, then update 
 * the 7-segment display if the pump is running with the elapsed "MIN:SEC"
 *
 * The 7-segment display always uses elapsedMinutes and never tries
 * to show hours b/c the pump should never be on continously longer
 * then 60-90minutes.
 */
void updateCounter() {
    if (pumpSwitch.isOn()) {
        uint32_t maxSeconds = pumpSwitch.getMaxOnMinutes() * 60UL;

        // flash the colon every 0.5s
        bool drawColon = (millis() % 1000 < 500) ? true : false;
        counter.drawColon(drawColon);

        // setup the countdoun value, use not-unsigned int so that we know
        // if it's gone negative (it's possible to out-of-sync by a little,
        // need to handle that edge case...)
        int32_t elapsedSeconds = maxSeconds - pumpSwitch.getElapsedSeconds();

        if (0 < elapsedSeconds) {
            uint32_t elapsedMinutes = (uint32_t)(elapsedSeconds / 60);
            counter.writeDigitNum(0, elapsedMinutes / 10, false);
            counter.writeDigitNum(1, elapsedMinutes % 10, false);

            uint32_t remainderSeconds = elapsedSeconds % 60;
            counter.writeDigitNum(3, remainderSeconds / 10, false);
            counter.writeDigitNum(4, remainderSeconds % 10, false);
        } else {
            // if the count goes negative, start flashing "00:00"
            if (drawColon) {
                counter.writeDigitNum(0, 0, false);
                counter.writeDigitNum(1, 0, false);
                counter.writeDigitNum(3, 0, false);
                counter.writeDigitNum(4, 0, false);
            } else {
                counter.clear();
            }
        }
    } else {
        // clear the display
        counter.clear();
    }

    counter.writeDisplay();
}

/*
 * Receive pumpValues from the base station
 */
void receivePumpSettings() {
    // TODO receive data from XBee radio
}

/*
 * Transmit pumpValues to the base station.
 */
void transmitPumpValues() {
    if (!lastTransmitTime || millis() - lastTransmitTime >= TRANSMIT_INTERVAL_SECONDS * 1000UL) {
        dbg("Sending data...");
        lastTransmitTime = millis();

        
        ZBTxRequest zbTx = ZBTxRequest(addr64, pumpSwitch.getPumpValues(), pumpSwitch.getNumPumpValues());
        xbee.send(zbTx);

        dbg("Data sent...");
        statusLed.flash(1, 100);

        if (xbee.readPacket(XBEE_STATUS_WAIT_MS)) {
            if (ZB_TX_STATUS_RESPONSE == xbee.getResponse().getApiId()) {
                xbee.getResponse().getZBTxStatusResponse(txStatus);

                if (SUCCESS == txStatus.getDeliveryStatus()) {
                    dbg("Success!");
                    statusLed.flash(5, 50);
                } else {
                    dbg("Failure :(");
                    statusLed.flash(3, 500);
                }
            }
        } else if (xbee.getResponse().isError()) {
            dbg("Error reading packet. Error code: %s", xbee.getResponse().getErrorCode());
        } else {
            dbg("Local XBee didn't provide a timely TX status response (should not happen)");
            statusLed.flash(2, 50);
        }

        dbg("Total transmit time: %lums", millis() - lastTransmitTime);
    }
}

void setup() {
    // hardware serial is used for FTDI debugging
    Debug.begin();

    dbg("setup() start..");

    setupUnusedPins();

    setupRelay();

    pumpSwitch.setup();

    setupCounter();

    setupXBee();

    dbg("setup() completed!");
}

void loop() {
    pumpSwitch.check();

    updateCounter();

    transmitPumpValues();
}

