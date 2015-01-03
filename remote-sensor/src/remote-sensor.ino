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
#include "Narcoleptic.h"

#include "Danaides.h"
#include "InputShiftRegister.h"
#include "LED.h"

/*
 * Constants
 */

// how often to check sensors
// NOTE: if all sensor values match previous values
//       then NO update will be transmitted.
#define CHECK_INTERVAL_SECONDS 10

// how often to transmit values, regardless of previous sensor values
#define FORCE_TRANSMIT_INTERVAL_SECONDS 30

// enable debug output
// 1 == ON
// 0 == OFF
#define DEBUG_ENABLED 1

/*
 * Pins
 */

#define INTERRUPT      1           // attach to the 2nd interrupt, which is pin 3
#define INTERRUPT_PIN  3           // interrupt pin (wake button)
#define CP_PIN         4           // 74HC165N Clock Pulse
#define CE_PIN         5           // 74HC165N Clock Enable
#define PL_PIN         6           // 74HC165N Parallel Load
#define NO_PIN_7       7           // no pin 7 on Trinket Pro boards
#define Q7_PIN         8           // 74HC165N Serial Out
#define UNUSED_PIN_9   9           // unused
#define UNUSED_PIN_10 10           // unused
#define UNUSED_PIN_11 11           // unused
#define UNUSED_PIN_12 12           // unused
#define STATUS_LED    13           // XBee Status LED
#define UNUSED_PIN_14 14           // unused   (Analog 0)
#define UNUSED_PIN_15 15           // unused   (Analog 1)
#define CTS_PIN       16           // XBee CTS (Analog 2)
#define SLEEP_PIN     17           // XBee DTR (Analog 3)
#define SS_TX_PIN     18           // XBee RX  (Analog 4)
#define SS_RX_PIN     19           // XBee TX  (Analog 5)

/*
 * Unused pins can drain power, set them to INPUT
 * w/internal PULLUP enabled to prevent power drain.
 */
void setupUnusedPins() {
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
ZBTxRequest zbTx = ZBTxRequest(addr64, sensorValues, sizeof(sensorValues));

ZBTxStatusResponse txStatus = ZBTxStatusResponse();

// XBee will use SoftwareSerial for communications, reserve the HardwareSerial
// for debugging w/FTDI interface.
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

LED statusLed = LED(STATUS_LED);

void setupXBee() {
    dbg("Setting up XBee...");

    pinMode(SLEEP_PIN, OUTPUT);
    digitalWrite(SLEEP_PIN, LOW);

    pinMode(CTS_PIN, INPUT);

    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);
    xbee.setSerial(ss);

    waitForXBeeWake();

    // XXX remove eventually, this consumes too much power
    // for battery use...
    statusLed.setup();

    // now sleep it until we need it
    sleepXBee();

    dbg("Completed XBee setup");
}

void wakeXBee() {
    dbg("Waking XBee...");
    digitalWrite(SLEEP_PIN, LOW);
    waitForXBeeWake();
}

void waitForXBeeWake() {
    // empirically, this usually takes 15-17ms
    // after waking from sleep
    uint32_t start = now();
    while (LOW != digitalRead(CTS_PIN)) {
        // delay until CTS_PIN goes low
        delay(1);
    }
    dbg("XBee wake latency: %lums", now() - start);
}

void sleepXBee() {
    dbg("Sleeping XBee...");
    digitalWrite(SLEEP_PIN, HIGH);
    // XXX investigate using SLEEP_PIN as INPUT instead, less power?
    // http://www.fiz-ix.com/2012/11/low-power-xbee-sleep-mode-with-arduino-and-pin-hibernation/
}

/*
 * InputShiftRegister
 */
InputShiftRegister inputs(SENSOR_TOTAL_INPUTS, PL_PIN, CE_PIN, CP_PIN, Q7_PIN);

/*
 * Arduino sleep
 */
void setupSleep() {
    //XXX investigate what can be disabled

    // never need this, leave it disabled all the time
    Narcoleptic.disableADC();
}

/*
 * Pin Interrupt (wake button)
 */
volatile bool interruptedByPin = false;
void pinInterrupt() {
    // when Narcoleptic is interrupted, it will still
    // return the same millis value as if it had completed
    // its full sleep interval. That's ok, we're not worried
    // about that loss of clock time.
    interruptedByPin = true;
}

void setupInterrupt() {
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(INTERRUPT, pinInterrupt, CHANGE);
}

/*
 * Remote Sensor (MAIN)
 */
uint32_t lastTransmitTime = 0;
bool sensorValuesChanged = false;

/*
 * Narcoleptic library tracks its total sleep time,
 * that can be used to correct millis().
 */
uint32_t now() {
    return millis() + Narcoleptic.millis();
}

/*
 * Show the current sensorValues as ON/OFF strings
 */
void displaySensorValues() {
    dbg("Sensor States:");

    for(int i = 0; i < SENSOR_TOTAL_INPUTS; i++)
    {
        if(1 == sensorValues[i]) {
            dbg("\tSensor %d:\t***ON***", i);
        } else {
            dbg("\tSensor %d:\tOFF", i);
        }
    }
}

/*
 * Update the sensorValues from the input shift register values
 */
void updateSensorValues() {
    // assume it's unchanged since the last update
    sensorValuesChanged = false;

    int* values = inputs.getValues();

    for (int i = 0; i < inputs.getNumInputs(); i++) {
        int value = values[i];

        if (TANK_1_INVERTED_FLOAT == i) {
            // invert the value so that its ON/OFF state is correct
            value = value ? 0 : 1;
        }

        if (sensorValues[i] != value) {
            sensorValuesChanged = true;
            sensorValues[i] = value;
        }
    }
}

/*
 * Check if the sensorValues changed, or FORCE_TRANSMIT_INTERVAL_SECONDS has elapsed
 * since the last sensorValues change, and transmit the sensorValues to the base station.
 */
void transmitSensorValues() {
    if (sensorValuesChanged || (now() - lastTransmitTime >= FORCE_TRANSMIT_INTERVAL_SECONDS * 1000) ) {
        dbg("Sending data...");
        lastTransmitTime = now();
        
        wakeXBee();

        // zbTx already has a reference to the sensorValues
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

        sleepXBee();

        dbg("Total transmit time: %lums", now() - lastTransmitTime);
    }
}

/*
 * Put the Arduino board into lowest-power sleep
 */
void sleepArduino() {
    dbg("Sleeping!");

    // allow serial buffer to flush before sleeping
    Serial.flush();

    Narcoleptic.delay(CHECK_INTERVAL_SECONDS * 1000);

    // continue from here after waking
    dbg("Awake!");
}

void setup() {
    // hardware serial is used for FTDI debugging
    Debug.begin();

    dbg("setup() start..");

    setupUnusedPins();

    setupInterrupt();

    setupSleep();

    inputs.setup();

    setupXBee();

    dbg("setup() completed!");

    dbg("Performing initial update & transmit");

    updateSensorValues();

    displaySensorValues();

    // force initial transmitting of values
    sensorValuesChanged = true;

    transmitSensorValues();
}

void loop() {
    sleepArduino();

    updateSensorValues();

    if (interruptedByPin) {
        interruptedByPin = false;
        displaySensorValues();
    }

    transmitSensorValues();
}

