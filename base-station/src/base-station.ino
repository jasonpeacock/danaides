/*
 * Receive data from the Remote Sensors and the
 * Pump Switch, display the data, and manage the
 * Pump Switch.
 *
 */

// system
#include <SoftwareSerial.h>

// third-party
#include "Bounce2.h"

// local
#include "Bargraph.h"
#include "Counter.h"
#include "Danaides.h"
#include "Display.h"
#include "LED.h"
#include "Message.h"
#include "PumpSwitch.h"
#include "TankSensors.h"
#include "WAN.h"

#include "pitches.h"

/*
 * Constants
 */

/*
 * Pins
 */

#define BUTTON_PIN     3           // Momentary switch
#define BUTTON_LED     4           // Momentary switch LED (blue)
#define EVALUATE_PIN   5           // Switch to allow BaseStation to manager PumpSwitch
#define UNUSED_PIN_6   6           // unused
#define NO_PIN_7       7           // no pin 7 on Trinket Pro boards
#define UNUSED_PIN_8   8           // unused
#define UNUSED_PIN_9   9           // unused
#define UNUSED_PIN_10 10           // unused
#define UNUSED_PIN_11 11           // unused
#define UNUSED_PIN_12 12           // unused
#define STATUS_LED    13           // XBee Status LED
#define UNUSED_PIN_14 14           // unused    (Analog 0)
#define SPEAKER_PIN   15           // Piezo Speaker (Analog 1)
#define SS_TX_PIN     16           // XBee RX   (Analog 2)
#define SS_RX_PIN     17           // XBee TX   (Analog 3)
#define I2C_DATA_PIN  18           // I2C Data  (Analog 4)
#define I2C_CLOCK_PIN 19           // I2C Clock (Analog 5)

/*
 * Unused pins can drain power, set them to INPUT
 * w/internal PULLUP enabled to prevent power drain.
 */
void setupUnusedPins() {
    pinMode(UNUSED_PIN_8,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_9,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_10, INPUT_PULLUP);
    pinMode(UNUSED_PIN_11, INPUT_PULLUP);
    pinMode(UNUSED_PIN_12, INPUT_PULLUP);

    pinMode(UNUSED_PIN_14, INPUT_PULLUP);
}

/*
 * Evaluate Switch
 */
Bounce evaluateSwitch = Bounce();
bool evaluateEnabled = true;
void setupEvaluate() {
    pinMode(EVALUATE_PIN, INPUT_PULLUP);
    evaluateSwitch.attach(EVALUATE_PIN);
    evaluateSwitch.interval(5); // ms

    evaluateSwitch.update();
    evaluateEnabled = evaluateSwitch.read();
}


/*
 * Piezo Speaker
 */
void setupSpeaker() {
    pinMode(SPEAKER_PIN, OUTPUT);
    noTone(SPEAKER_PIN);
}

void playSetupMelody() {
    // note durations: 4 = quarter note, 8 = eighth note, etc.:
    uint8_t noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};
    uint16_t melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
    uint8_t numNotes = 8;

    // iterate over the notes of the melody:
    for (uint8_t thisNote = 0; thisNote < numNotes; thisNote++) {

        // to calculate the note duration, take one second
        // divided by the note type.
        //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        uint16_t noteDuration = 1000 / noteDurations[thisNote];
        tone(SPEAKER_PIN, melody[thisNote], noteDuration);

        // to distinguish the notes, set a minimum time between them.
        // the note's duration + 30% seems to work well:
        delay(noteDuration * 1.30);

        // stop the tone playing:
        noTone(SPEAKER_PIN);
    }
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
 * Base Station (MAIN)
 */
PumpSwitch pumpSwitch = PumpSwitch(BUTTON_PIN, BUTTON_LED, enablePump, disablePump);
TankSensors tankSensors = TankSensors();

Message message = Message(0x70, 0x71);
Counter counter = Counter(0x72);
Bargraph bar_1 = Bargraph(0x73);
Bargraph bar_2 = Bargraph(0x74);
Bargraph bar_3 = Bargraph(0x75);

Display display = Display(message, counter, bar_1, bar_2, bar_3);

void enablePump() {
    Serial.println(F("Pump enabled!"));
    // send the current (updated) pump values
    transmit();
    display.scroll("PUMP ENABLED");

}

void disablePump() {
    Serial.println(F("Pump disabled!"));
    // send the current (updated) pump values
    transmit();
    display.scroll("PUMP DISABLED");
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
            // we're the base station, this would be weird
        } else if (wan.isRemoteSensorAddress(data.getAddress())) {
            Serial.println(F("New data from Remote Sensor"));
            if (tankSensors.getNumSensors() == data.getSize()) {
                tankSensors.update(data);
                Serial.println(F("Updated Tank Sensor values"));
            }
        } else if (wan.isPumpSwitchAddress(data.getAddress())) {
            Serial.println(F("New data from Pump Switch"));
            if (data.getSize() == pumpSwitch.getNumValues()) {
                // update the local PumpSwitch object, the remote switch is always
                // the authority for the values & settings
                if (pumpSwitch.getNumSettings() == data.getSize()) {
                    pumpSwitch.updateSettings(data.getData(), data.getSize());
                    Serial.println(F("Updated Pump Switch settings"));
                } else if (pumpSwitch.getNumValues() == data.getSize()) {
                    pumpSwitch.updateValues(data.getData(), data.getSize());
                    Serial.println(F("Updated Pump Switch values"));
                }
            }
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

void transmit() {
    uint32_t start = millis();

    Serial.println(F("transmitting..."));
    Serial.flush();

    freeRam();

    Data values = Data(wan.getPumpSwitchAddress(), pumpSwitch.getValues(), pumpSwitch.getNumValues());
    
    if (wan.transmit(&values)) {
        Serial.println(F("PumpSwitch data sent!"));
    } else {
        Serial.println(F("Failed to transmit Pump Switch data"));
    }

    freeRam();

    Serial.print(F("Total transmit time (ms): "));
    Serial.println(millis() - start);
    Serial.flush();
}

uint32_t lastTankSensorCheckTime = 0UL;
void evaluateTankSensors() {
    if (millis() - lastTankSensorCheckTime > EVALUATE_TANK_SENSOR_INTERVAL_MINUTES * 60UL * 1000UL) {
        lastTankSensorCheckTime = millis();

        if (!tankSensors.ready()) {
            Serial.println(F("TankSensors not updated yet"));
            return;
        }

        Serial.print(F("Checking tank sensors..."));
        Serial.print(F("Tank 1 state: "));
        Serial.print(tankSensors.getTankState(1));
        Serial.print(F(" Pump state: "));
        Serial.println(pumpSwitch.isOn());

        if (!evaluateEnabled) {
            Serial.println(F("Evaluate disabled, not doing anything"));
            return;
        }

        if (tankSensors.getTankState(1) && pumpSwitch.isOn()) {
            // tank #1 is FULL and pump is ON, turn the pump OFF
            // ...if the pump has not run long enough (MIN_ON_MINUTES)
            // then the switch will ignore this request
            Serial.println(F("Tank 1 full, stopping pump"));
            pumpSwitch.stop();
        } else if (!tankSensors.getTankState(1) && pumpSwitch.isOff()) {
            // tank #1 is NOT FULL and pump is OFF, turn the pump ON
            // ...if the pump has not rested long enough (MIN_OFF_MINUTES)
            // then the switch will ignore this request
            Serial.println(F("Tank 1 NOT full, starting pump"));
            pumpSwitch.start();
        }
    }
}

void setup() {
    // hardware serial is used for FTDI debugging
    Serial.begin(9600);

    Serial.println(F("setup() start.."));

    setupUnusedPins();

    setupSpeaker();

    pumpSwitch.setup();

    display.setup();

    setupWAN();

    setupEvaluate();

    Serial.println(F("setup() completed!"));

    Serial.println(F("Performing initial update & transmit"));

    // TODO transmit any initial values

    // say something
    display.scroll("HELLO");

    // play a sound
    //playSetupMelody();

    freeRam();
}

void loop() {
    pumpSwitch.check();
    wan.check();

    display.check(pumpSwitch, tankSensors);

    if (evaluateSwitch.update()) {
        // switch was switched
        evaluateEnabled = evaluateSwitch.read();
        Serial.print(F("Evaluate switch updated: "));
        Serial.println(evaluateEnabled);
    }

    receive();

    evaluateTankSensors();
}

