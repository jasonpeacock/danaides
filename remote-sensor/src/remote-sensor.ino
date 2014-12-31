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
#define UNUSED_PIN_10 10           // unused
#define UNUSED_PIN_11 11           // unused
#define UNUSED_PIN_12 12           // unused
#define LED           13           // pin 13 == LED_BUILTIN
#define STATUS_LED_PIN LED_BUILTIN // XBee Status LED
#define ERROR_LED_PIN  LED_BUILTIN // XBee Error LED
#define UNUSED_PIN_14 14           // unused (Analog 0)
#define UNUSED_PIN_15 15           // unused (Analog 1)
#define CTS_PIN       16           // XBee CTS (Analog 2)
#define SLEEP_PIN     17           // XBee SP (DTR) (Analog 3)
#define SS_TX_PIN     18           // XBee RX (Analog 4)
#define SS_RX_PIN     19           // XBee TX (Analog 5)

/*
 * Unused pins can drain power, set them to INPUT
 * w/internal PULLUP enabled to prevent power drain.
 */
void setupUnusedPins() {
    pinMode(UNUSED_PIN_3,  INPUT_PULLUP);

    pinMode(UNUSED_PIN_9,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_10, INPUT_PULLUP);
    pinMode(UNUSED_PIN_11, INPUT_PULLUP);
    pinMode(UNUSED_PIN_12, INPUT_PULLUP);

    pinMode(UNUSED_PIN_14, INPUT_PULLUP);
    pinMode(UNUSED_PIN_15, INPUT_PULLUP);
}

/*
 * Remote Sensor Constants
 */

// how often to check sensors
// NOTE: if all sensor values match previous values
//       then NO update will be transmitted.
#define CHECK_INTERVAL_SECONDS 10

// how often to transmit values, regardless of previous sensor values
#define FORCE_TRANSMIT_INTERVAL_SECONDS 30

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

/*
 * Setup Arduino sleep
 */
unsigned long lastTransmitTime = 0;
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

    pinMode(CTS_PIN, INPUT);

    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);
    xbee.setSerial(ss);

    delay(XBEE_SETUP_DELAY_SECONDS * 1000);

    // now sleep it until we need it
    sleepXBee();

    dbg("Completed XBee setup");
}

void wakeXBee() {
    dbg("Waking XBee...");
    digitalWrite(SLEEP_PIN, LOW);

    // wait for radio to be ready to receive input
    // empirically, this usually takes 15-17ms
    unsigned long start = now();
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
 * Check if the payload changed, or FORCE_TRANSMIT_INTERVAL_SECONDS has elapsed
 * since the last payload change, and transmit the payload to the base station.
 */
void transmitPayload() {
    if (payloadChanged || (now() - lastTransmitTime >= FORCE_TRANSMIT_INTERVAL_SECONDS * 1000) ) {
        dbg("Sending data...");
        lastTransmitTime = now();
        
        wakeXBee();

        // zbTx already has a reference to the payload
        xbee.send(zbTx);

        dbg("Data sent...");
        flashLed(STATUS_LED_PIN, 1, 100);

        if (xbee.readPacket(STATUS_WAIT_MS)) {
            if (ZB_TX_STATUS_RESPONSE == xbee.getResponse().getApiId()) {
                xbee.getResponse().getZBTxStatusResponse(txStatus);

                if (SUCCESS == txStatus.getDeliveryStatus()) {
                    dbg("Success!");
                    flashLed(STATUS_LED_PIN, 5, 50);
                } else {
                    dbg("Failure :(");
                    flashLed(ERROR_LED_PIN, 3, 500);
                }
            }
        } else if (xbee.getResponse().isError()) {
            dbg("Error reading packet. Error code: %s", xbee.getResponse().getErrorCode());
        } else {
            dbg("Local XBee didn't provide a timely TX status response (should not happen)");
            flashLed(ERROR_LED_PIN, 2, 50);
        }


        // sleep once we get our successful ack response
        sleepXBee();

        dbg("Total transmit time: %lums", now() - lastTransmitTime);
    }
}

unsigned long now() {
    // adjust for delays from sleeping
    return millis() + Narcoleptic.millis();
}

void sleepArduino() {
    // XXX allow serial buffer to flush before sleeping
    dbg("Sleeping!");
    delay(1 * 1000);

    Narcoleptic.delay(CHECK_INTERVAL_SECONDS * 1000);

    // continue from here after waking
    dbg("Awake!");
}

void setupSleep() {
    // never need this, leave it disabled all the time
    Narcoleptic.disableADC();
}

void setup() {
    // hardware serial is used for FTDI debugging
    Debug.begin();

    dbg("setup() start..");

    setupUnusedPins();

    setupSleep();

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
    sleepArduino();

    updatePayload();

    displayPayload();

    transmitPayload();
}

