/*
 * Report the tank float switch, valve position,
 * and flow meter sensor values periodically.
 *
 * Aggressively sleep between reporting periods
 * to conserve battery power, only transmitting
 * when values change from previous readings.
 *
 * Display the current sensor values when the
 * momentary switch is pressed (interrupt-awake
 * on the Trinket Pro).
 * 
 */

#include <SoftwareSerial.h>

#include "XBee.h"
#include "Dbg.h"

#include "Danaides.h"
#include "InputShiftRegister.h"

/*
 * Pins
 */

#define UNUSED_PIN_3   3           // unused
#define CP_PIN         4           // 74HC165N Clock Pulse
#define CE_PIN         5           // 74HC165N Clock Enable
#define PL_PIN         6           // 74HC165N Parallel Load
#define NO_PIN_7       7           // no pin 7 on Trinket Pro boards
#define Q7_PIN         8           // 74HC165N Serial Out
#define UNUSED_PIN_9   9           // unused
#define SLEEP_PIN     10           // XBee SP (DTR)
#define SS_TX_PIN     11           // XBee RX
#define SS_RX_PIN     12           // XBee TX
#define LED           13           // pin 13 == LED_BUILTIN
#define STATUS_LED_PIN LED_BUILTIN // XBee Status LED
#define ERROR_LED_PIN  LED_BUILTIN // XBee Error LED

/*
 * Remote Sensor Constants
 */

// how often to check sensors
// NOTE: if all sensor values match previous values
//       then NO update will be transmitted.
#define CHECK_INTERVAL_SECONDS 10UL

// how often to transmit values, regardless of previous sensor values
#define FORCE_TRANSMIT_INTERVAL_SECONDS 30UL

// enable debugging: 1 == ON, 0 == OFF
#define DEBUG_ENABLED 1

/*
 * Setup the XBee radio
 */ 
XBee xbee = XBee();

XBeeAddress64 addr64 = XBeeAddress64(XBEE_FAMILY_ADDRESS, BASE_STATION_ADDRESS);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));

ZBTxStatusResponse txStatus = ZBTxStatusResponse();

// XBee will use SoftwareSerial for communications, reserver the HardwareSerial
// for debugging w/FTDI interface.
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

/*
 * Setup the InputShiftRegister
 */
InputShiftRegister inputs(TOTAL_SENSOR_INPUTS, PL_PIN, CE_PIN, CP_PIN, Q7_PIN);

unsigned long lastTransmitTime = 0;
bool lastTransmitSuccess = false;
bool payloadChanged = false;

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

void displayPayload() {
    dbg("Sensor States:");

    for(int i = 0; i < TOTAL_SENSOR_INPUTS; i++)
    {
        if(1 == payload[i]) {
            dbg("\tSensor %d:\t***ON***", i);
        } else {
            dbg("\tSensor %d:\tOFF", i);
        }
    }
}


void setupXBee() {
    dbg("Setting up XBee...");

    // XXX remove eventually, this consumes too much power
    // for battery use...
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(ERROR_LED_PIN, OUTPUT);

    pinMode(SLEEP_PIN, OUTPUT);
    digitalWrite(SLEEP_PIN, LOW);

    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);
    xbee.setSerial(ss);

    delay(XBEE_SETUP_DELAY_SECONDS * 1000);

    dbg("Completed XBee setup");
}

void wakeXBee() {
    //XXX fix the sleep
    //digitalWrite(SLEEP_PIN, LOW);
    // wait for radio to be ready to receive input
    // XXX use CTS to detect when ready?
    delay(50);
}

void sleepXBee() {
    //XXX fix the sleep
    //digitalWrite(SLEEP_PIN, HIGH);
    // XXX investigate using SLEEP_PIN as INPUT instead, less power?
    // http://www.fiz-ix.com/2012/11/low-power-xbee-sleep-mode-with-arduino-and-pin-hibernation/
}

/*
 * Update the payload from the input shift register values
 */
void updatePayload() {
    // assume it's unchanged since the last update
    payloadChanged = false;

    int* values = inputs.getValues();

    for (int i = 0; i < inputs.getNumInputs(); i++) {
        int value = values[i];

        if (TANK_1_INVERTED_FLOAT == i) {
            // invert the value so that its ON/OFF state is correct
            value = value ? 0 : 1;
        }

        if (payload[i] != value) {
            payloadChanged = true;
            payload[i] = value;
        }
    }
}

/*
 * Check if the payload changed, or FORCE_TRANSMIT_INTERVAL_SECONDS has elapsed,
 * and transmit the payload to the base station.
 */
void transmitPayload() {
    if (payloadChanged || (unsigned long)(millis() - lastTransmitTime) >= FORCE_TRANSMIT_INTERVAL_SECONDS * 1000) {
        lastTransmitTime = millis();

        wakeXBee();

        dbg("Sending data...");
        // send the data! zbTx already has a reference to the payload
        xbee.send(zbTx);

        delay(XBEE_TRANSMIT_DELAY_SECONDS);

        dbg("Data sent!");
        flashLed(STATUS_LED_PIN, 1, 100);

        if (xbee.readPacket(STATUS_WAIT_MS)) {
            if (ZB_TX_STATUS_RESPONSE == xbee.getResponse().getApiId()) {
                dbg("ZB_TX_STATUS_RESPONSE");
                xbee.getResponse().getZBTxStatusResponse(txStatus);

                if (SUCCESS == txStatus.getDeliveryStatus()) {
                    lastTransmitSuccess = true;
                    dbg("Success!");
                    flashLed(STATUS_LED_PIN, 5, 50);
                } else {
                    lastTransmitSuccess = false;
                    dbg("Failure :(");
                    flashLed(ERROR_LED_PIN, 3, 500);
                }
            }
        } else if (xbee.getResponse().isError()) {
            lastTransmitSuccess = false;
            dbg("Error reading packet. Error code: %s", xbee.getResponse().getErrorCode());
        } else {
            lastTransmitSuccess = false;
            dbg("Local XBee didn't provide a timely TX status response (should not happen)");
            flashLed(ERROR_LED_PIN, 2, 50);
        }

        sleepXBee();
    }
}

void sleep() {
    dbg("Waiting %ds before doing it all again...", CHECK_INTERVAL_SECONDS);

    // XXX go to sleep here to save battery power
    delay(CHECK_INTERVAL_SECONDS * 1000);
}

void setup() {
    // hardware serial is used for FTDI debugging
    Debug.begin();

    dbg("setup() start..");

    inputs.setup();
    setupXBee();

    // get the initial values at startup
    updatePayload();
    displayPayload();

    // force initial transmitting of values
    payloadChanged = true;
    transmitPayload();

    dbg("setup() completed!");
}

void loop() {

    updatePayload();

    displayPayload();

    transmitPayload();

    sleep();
}

