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
#include "Dbg.h"
#include "XBee.h"

// local
#include "Danaides.h"
#include "LED.h"
#include "PumpSwitch.h"
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

// how often to transmit values to pump switch
#define TRANSMIT_INTERVAL_SECONDS 60

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
 * Send messages to pump-switch
 */
void enablePump() {
    dbg("Pump enabled!");
    scrollAlphaMessage("PUMP ENABLED", 1);
}

void disablePump() {
    dbg("Pump disabled!");
    scrollAlphaMessage("PUMP DISABLED", 1);
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

void scrollAlphaMessage(char* message, uint8_t numScrolls) {
    uint8_t messageSize = strlen(message); // max 255 chars
    char displayBuffer[4] = {' ', ' ', ' ', ' '};

    while(numScrolls > 0) {
        numScrolls--;
        
        for (uint8_t i = 0; i < messageSize + 4; i++) {
            // scroll the message off the display by including
            // 4 extra empty chars
            char c = message[i];
            if (i >= messageSize) {
                c = ' ';
            }

            displayBuffer[0] = displayBuffer[1];
            displayBuffer[1] = displayBuffer[2];
            displayBuffer[2] = displayBuffer[3];
            displayBuffer[3] = c;

            alpha.writeDigitAscii(0, displayBuffer[0]);
            alpha.writeDigitAscii(1, displayBuffer[1]);
            alpha.writeDigitAscii(2, displayBuffer[2]);
            alpha.writeDigitAscii(3, displayBuffer[3]);

            alpha.writeDisplay();
            delay(200);
        }
    }
}

/*
 * Base Station (MAIN)
 */
PumpSwitch pumpSwitch = PumpSwitch(BUTTON_PIN, BUTTON_LED, enablePump, disablePump);

void receive() {
    uint32_t lastReceiveTime = millis();

    Data data = Data();
    if (wan.receive(data)) {
        dbg("Received data...");
        Serial.flush();

        uint32_t address = data.getAddress();
        if (wan.isBaseStationAddress(address)) {
            dbg("New data from Base Station (0x%lX)", address);
            // we're the base station, this would be weird
        } else if (wan.isRemoteSensorAddress(address)) {
            dbg("New data from Remote Sensor (0x%lX)", address);
            // TODO update the display
        } else if (wan.isPumpSwitchAddress(address)) {
            dbg("New data from Pump Switch (0x%lX)", address);
            if (data.getSize() == pumpSwitch.getNumValues()) {
                // update the local PumpSwitch object, the remote switch is always
                // the authority for the values (pump state & elapsed time)
                pumpSwitch.updateValues(data.getData(), data.getSize());
                dbg("Updated Pump Switch values");
            }
        }

        for (uint8_t i = 0; i < data.getSize(); i++) {
            dbg("%d:\t%d", i, data.getData()[i]);
        }

        dbg("Total receive time: %dms", millis() - lastReceiveTime);
        Serial.flush();
    }
}

uint32_t lastTransmitTime = 0;
void transmit() {
    if (!lastTransmitTime || millis() - lastTransmitTime > TRANSMIT_INTERVAL_SECONDS * 1000UL) {
        lastTransmitTime = millis();

        dbg("transmitting...");
        Serial.flush();

        Data settings = Data(wan.getPumpSwitchAddress(), pumpSwitch.getSettings(), pumpSwitch.getNumSettings());
        
        if (wan.transmit(&settings)) {
            dbg("PumpSwitch data sent!");
        } else {
            dbg("Failed to transmit Pump Switch data");
        }

        dbg("Total transmit time: %dms", millis() - lastTransmitTime);
        Serial.flush();
    }
}

void setup() {
    // hardware serial is used for FTDI debugging
    Debug.begin();

    dbg("setup() start..");

    setupUnusedPins();

    setupSpeaker();

    pumpSwitch.setup();

    setupAlpha();

    setupWAN();

    dbg("setup() completed!");

    dbg("Performing initial update & transmit");

    // TODO transmit any initial values

    // say something
    scrollAlphaMessage("HELLO", 1);

    // play a sound
    playSetupMelody();
}

void loop() {
    pumpSwitch.check();
    
    receive();

    transmit();
}
