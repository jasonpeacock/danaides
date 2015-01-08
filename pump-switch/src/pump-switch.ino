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

// system
#include <SoftwareSerial.h>

// third-party
#include "Adafruit_LEDBackpack.h"

// local
#include "Danaides.h"
#include "LED.h"
#include "PumpSwitch.h"
#include "WAN.h"

/*
 * Constants
 */

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

// how often to transmit values
// TODO update with proper value...
#define TRANSMIT_INTERVAL_SECONDS 10

// XBee will use SoftwareSerial for communications, reserve the HardwareSerial
// for debugging w/FTDI interface.
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

LED statusLed = LED(STATUS_LED);

WAN wan = WAN(ss, statusLed);

void setupWAN() {
    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);

    wan.setup();
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
 * Pump Switch (MAIN)
 */
PumpSwitch pumpSwitch = PumpSwitch(BUTTON_PIN, BUTTON_LED, enableRelay, disableRelay);

void receive() {
    uint32_t lastReceiveTime = millis();

    Data data = Data();
    if (wan.receive(data)) {
        Serial.println(F("Received data..."));
        Serial.flush();

        freeRam();

        if (wan.isBaseStationAddress(data.getAddress())) {
            Serial.println(F("New data from Base Station"));
            if (data.getSize() == pumpSwitch.getNumValues()) {
                // update the local PumpSwitch object, the base station may have
                // enabled/disabled the pump
                pumpSwitch.updateValues(data.getData(), data.getSize());
                Serial.println(F("Updated Pump Switch values"));
            }
        } else if (wan.isRemoteSensorAddress(data.getAddress())) {
            Serial.println(F("New data from Remote Sensor"));
            // nothing to do
        } else if (wan.isPumpSwitchAddress(data.getAddress())) {
            Serial.println(F("New data from Pump Switch"));
            // we're the pump switch, this would be weird
        }

        for (uint8_t i = 0; i < data.getSize(); i++) {
            Serial.print(i);
            Serial.print(F(":\t"));
            Serial.println(data.getData()[i]);
        }

        freeRam();

        Serial.print(F("Total receive time (ms): "));
        Serial.println(millis() - lastReceiveTime);
        Serial.flush();
    }
}

/*
 * Transmit Values & Settings to the base station.
 */
uint32_t lastTransmitTime = 0;
bool lastTransmittedSettings = false;
void transmit(bool forceValues = false) {
    if (forceValues || !lastTransmitTime || millis() - lastTransmitTime > TRANSMIT_INTERVAL_SECONDS * 1000UL) {
        lastTransmitTime = millis();

        Serial.println(F("transmitting..."));
        Serial.flush();

        freeRam();

        // alternate between sending the values & sending the settings,
        // don't send them back-to-back b/c base-station may not process
        // the incoming messages fast enough.
        if (forceValues || lastTransmittedSettings) {
            lastTransmittedSettings = false;

            Data values = Data(wan.getBaseStationAddress(), pumpSwitch.getValues(), pumpSwitch.getNumValues());
            if (wan.transmit(&values)) {
                Serial.println(F("PumpSwitch values sent!"));
            } else {
                Serial.println(F("Failed to transmit values"));
            }
        } else {
            lastTransmittedSettings = true;

            Data settings = Data(wan.getBaseStationAddress(), pumpSwitch.getSettings(), pumpSwitch.getNumSettings());
            if (wan.transmit(&settings)) {
                Serial.println(F("PumpSwitch settings sent!"));
            } else {
                Serial.println(F("Failed to transmit settings"));
            }
        }

        freeRam();

        Serial.print(F("Total transmit time (ms): "));
        Serial.println(millis() - lastTransmitTime);
        Serial.flush();
    }
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
    // send the current (updated) pump values
    transmit(true);
}

void disableRelay() {
    digitalWrite(PUMP_RELAY_PIN, LOW);
    // send the current (updated) pump values
    transmit(true);
}

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

void setup() {
    // hardware serial is used for FTDI debugging
    Serial.begin(9600);

    Serial.println(F("setup() start.."));

    setupUnusedPins();

    setupRelay();

    pumpSwitch.setup();

    setupCounter();

    setupWAN();

    Serial.println(F("setup() completed!"));
}

void loop() {
    pumpSwitch.check();

    updateCounter();

    receive();

    transmit();
}

