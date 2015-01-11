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
#include "Counter.h"
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
 * Pump Switch (MAIN)
 */
PumpSwitch pumpSwitch = PumpSwitch(true, BUTTON_PIN, BUTTON_LED, enableRelay, disableRelay);
Counter counter = Counter(0x70);

// calculate the remaining pump run time and display it
void updateCounter() {
    int32_t elapsedSeconds = pumpSwitch.getMaxOnMinutes() * 60UL - pumpSwitch.getElapsedSeconds();
    counter.check(pumpSwitch.isOn(), elapsedSeconds);
}

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
    if (forceValues || !lastTransmitTime || millis() - lastTransmitTime > PUMP_SWITCH_TRANSMIT_INTERVAL_SECONDS * 1000UL) {
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

void setup() {
    // hardware serial is used for FTDI debugging
    Serial.begin(9600);

    Serial.println(F("setup() start.."));

    setupUnusedPins();

    setupRelay();

    pumpSwitch.setup();
    
    counter.setup();

    setupWAN();

    Serial.println(F("setup() completed!"));
}

void loop() {
    pumpSwitch.check();

    updateCounter();

    receive();

    transmit();
}

