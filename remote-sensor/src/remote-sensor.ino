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

// system
#include <SoftwareSerial.h>

// third-party
#include "XBee.h"
#include "Narcoleptic.h"

// local
#include "Danaides.h"
#include "InputShiftRegister.h"
#include "LED.h"
#include "WAN.h"

/*
 * Constants
 */

// how often to check sensors
// NOTE: if all sensor values match previous values
//       then NO update will be transmitted.
#define CHECK_INTERVAL_SECONDS 10

// how often to transmit values, regardless of previous sensor values
#define FORCE_TRANSMIT_INTERVAL_SECONDS 30

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
 * Narcoleptic library tracks its total sleep time,
 * that can be used to correct millis().
 */
uint32_t now() {
    return millis() + Narcoleptic.millis();
}

/*
 * XBee
 */ 

// XBee will use SoftwareSerial for communications, reserve the HardwareSerial
// for debugging w/FTDI interface.
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

// XXX remove eventually, this consumes too much power
// for battery use...
LED statusLed = LED(STATUS_LED);

WAN wan = WAN(ss, statusLed);

void setupWAN() {
    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);

    wan.setup();

    wan.enableSleep(SLEEP_PIN, CTS_PIN);
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
uint32_t lastTransmitTime = 0UL;
bool sensorValuesChanged = false;

/*
 * Update the sensorValues from the input shift register values
 */
void updateSensorValues() {
    sensorValuesChanged = inputs.getValues(sensorValues);
}

/*
 * Show the current sensorValues as ON/OFF strings
 */
void displaySensorValues() {
    Serial.println(F("Sensor States:"));

    for(uint8_t i = 0; i < SENSOR_TOTAL_INPUTS; i++)
    {
        Serial.print(F("\tSensor "));
        Serial.print(i);
        if(1 == sensorValues[i]) {
            Serial.println(F("\t***ON***"));
        } else {
            Serial.println(F("\tOFF"));
        }
    }
}

/*
 * Check if the sensorValues changed, or FORCE_TRANSMIT_INTERVAL_SECONDS has elapsed
 * since the last sensorValues change, and transmit the sensorValues to the base station.
 */
void transmitSensorValues() {
    if (sensorValuesChanged || now() - lastTransmitTime >= FORCE_TRANSMIT_INTERVAL_SECONDS * 1000UL) {
        Serial.println(F("Sending data..."));
        lastTransmitTime = now();
        
        Data values = Data(wan.getBaseStationAddress(), sensorValues, SENSOR_TOTAL_INPUTS);
        if (wan.transmit(&values)) {
            Serial.println(F("Sensor values sent!"));
        } else {
            Serial.println(F("Failed to transmit values"));
        }

        Serial.print(F("Total transmit time (ms): "));
        Serial.println(now() - lastTransmitTime);
    }
}

/*
 * Put the Arduino board into lowest-power sleep
 */
void sleepArduino() {
    Serial.println(F("Sleeping!"));

    // allow serial buffer to flush before sleeping
    Serial.flush();

    Narcoleptic.delay(CHECK_INTERVAL_SECONDS * 1000);

    // continue from here after waking
    Serial.println(F("Awake!"));
}

void setup() {
    // hardware serial is used for FTDI debugging
    Serial.begin(9600);

    Serial.println(F("setup() start.."));

    setupUnusedPins();

    setupInterrupt();

    setupSleep();

    inputs.setup();

    setupWAN();

    Serial.println(F("setup() completed!"));

    Serial.println(F("Performing initial update & transmit"));

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

