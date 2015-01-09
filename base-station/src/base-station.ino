/*
 * Receive data from the Remote Sensors and the
 * Pump Switch, display the data, and manage the
 * Pump Switch.
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
#define UNUSED_PIN_5   5           // unused
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
    pinMode(UNUSED_PIN_5,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_6,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_8,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_9,  INPUT_PULLUP);
    pinMode(UNUSED_PIN_10, INPUT_PULLUP);
    pinMode(UNUSED_PIN_11, INPUT_PULLUP);
    pinMode(UNUSED_PIN_12, INPUT_PULLUP);

    pinMode(UNUSED_PIN_14, INPUT_PULLUP);
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
 * Adafruit 7-segment LED Numeric display
 */
Adafruit_AlphaNum4 alpha = Adafruit_AlphaNum4();

void setupAlpha() {
    alpha.begin(0x70); // default address for first/single display
    alpha.clear();
    alpha.writeDisplay();
}

#define SCROLL_CHAR_COUNT 4
uint32_t lastScrollTime = 0UL;
uint8_t scrollPosition = 0;
char scrollBuffer[SCROLL_CHAR_COUNT] = {' ', ' ', ' ', ' '};
char scrollMessage[250];
void setScrollMessage(char* message) {
    // copy the message
    strncpy(scrollMessage, message, 249);
    // just in case message was larger...
    scrollMessage[249] = '\0';

    // clear the buffer
    for (uint8_t i = 0; i < SCROLL_CHAR_COUNT; i++) {
        scrollBuffer[i] = ' ';
    }

    // clear the display
    alpha.clear();
    alpha.writeDisplay();

    // reset the vars
    scrollPosition = 0;
    lastScrollTime = 0UL;
}

#define SCROLL_DELAY_MILLIS 200UL
void updateScrollMessage() {
    uint8_t size = strlen(scrollMessage);
    if (size + SCROLL_CHAR_COUNT <= scrollPosition) {
        // everything has been scrolled, nothing to do
        return;
    }

    if (millis() - lastScrollTime > SCROLL_DELAY_MILLIS) {
        // scroll the message off the display by including
        // 4 extra empty chars
        char c = scrollMessage[scrollPosition];
        if (size <= scrollPosition) {
            c = ' ';
        }

        for (uint8_t i = 0; i < SCROLL_CHAR_COUNT; i++) {
            if (i == SCROLL_CHAR_COUNT - 1) {
                scrollBuffer[i] = c;
            } else {
                scrollBuffer[i] = scrollBuffer[i + 1];
            }

            alpha.writeDigitAscii(i, scrollBuffer[i]);
        }

        alpha.writeDisplay();

        lastScrollTime = millis();
        scrollPosition++;
    }
}

/*
 * Base Station (MAIN)
 */
PumpSwitch pumpSwitch = PumpSwitch(BUTTON_PIN, BUTTON_LED, enablePump, disablePump);

TankSensors tankSensors = TankSensors();

uint32_t lastStatusTime = 0;

void enablePump() {
    Serial.println(F("Pump enabled!"));
    // send the current (updated) pump values
    transmit();
    setScrollMessage("PUMP ENABLED");

    // reset the status display so we can show our current
    // message before it displays itself.
    lastStatusTime = millis();
}

void disablePump() {
    Serial.println(F("Pump disabled!"));
    // send the current (updated) pump values
    transmit();
    setScrollMessage("PUMP DISABLED");

    // reset the status display so we can show our current
    // message before it displays itself.
    lastStatusTime = millis();
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

                // TODO update the display
            }
        } else if (wan.isPumpSwitchAddress(data.getAddress())) {
            Serial.println(F("New data from Pump Switch"));
            if (data.getSize() == pumpSwitch.getNumValues()) {
                // update the local PumpSwitch object, the remote switch is always
                // the authority for the values & settings
                if (PUMP_SETTINGS_TOTAL == data.getSize()) {
                    pumpSwitch.updateSettings(data.getData(), data.getSize());
                    Serial.println(F("Updated Pump Switch settings"));
                } else if (PUMP_VALUES_TOTAL == data.getSize()) {
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

void displayStatus() {
    if (millis() - lastStatusTime > 15 * 1000UL) {
        lastStatusTime = millis();

        char state[4];
        if (pumpSwitch.isOn()) {
            snprintf(state, 4, "ON");
        } else {
            snprintf(state, 4, "OFF");
        }

        char duration_1[15];
        char duration_2[15];
        if (pumpSwitch.getValues()[PUMP_VALUES_DAYS]) {
            snprintf(duration_1, 15, "%u DAYS", pumpSwitch.getValues()[PUMP_VALUES_DAYS]);
            snprintf(duration_2, 15, "%u HOURS", pumpSwitch.getValues()[PUMP_VALUES_HOURS]);
        } else if (pumpSwitch.getValues()[PUMP_VALUES_HOURS]) {
            snprintf(duration_1, 15, "%u HOURS", pumpSwitch.getValues()[PUMP_VALUES_HOURS]);
            snprintf(duration_2, 15, "%u MINUTES", pumpSwitch.getValues()[PUMP_VALUES_MINUTES]);
        } else {
            snprintf(duration_1, 15, "%u MINUTES", pumpSwitch.getValues()[PUMP_VALUES_MINUTES]);
            snprintf(duration_2, 15, "%u SECONDS", pumpSwitch.getValues()[PUMP_VALUES_SECONDS]);
        }

        char message[100];
        snprintf(message, 100, "PUMP STATUS %s %s %s", state, duration_1, duration_2);
        setScrollMessage(message);
    }
}

void setup() {
    // hardware serial is used for FTDI debugging
    Serial.begin(9600);

    Serial.println(F("setup() start.."));

    setupUnusedPins();

    setupSpeaker();

    pumpSwitch.setup();

    tankSensors.setup();

    setupAlpha();

    setupWAN();

    Serial.println(F("setup() completed!"));

    Serial.println(F("Performing initial update & transmit"));

    // TODO transmit any initial values

    // say something
    setScrollMessage("HELLO");

    // play a sound
    //playSetupMelody();

    freeRam();
}

void loop() {
    updateScrollMessage();

    pumpSwitch.check();
    
    tankSensors.check();
    
    receive();

    displayStatus();
}
