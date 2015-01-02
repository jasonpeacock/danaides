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

#include "XBee.h"
#include "Dbg.h"
#include "Bounce2.h"
#include "Adafruit_LEDBackpack.h"

#include "Danaides.h"

/*
 * Constants
 */

// how often to check pump elapsed time
#define CHECK_INTERVAL_SECONDS 5

// how often to transmit values, regardless of previous sensor values
#define FORCE_TRANSMIT_INTERVAL_SECONDS 60

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
#define LED           13           // pin 13 == LED_BUILTIN
#define STATUS_LED_PIN LED_BUILTIN // XBee Status LED
#define ERROR_LED_PIN  LED_BUILTIN // XBee Error LED
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
ZBTxRequest zbTx = ZBTxRequest(addr64, pumpValuesOut, sizeof(pumpValuesOut));

ZBTxStatusResponse txStatus = ZBTxStatusResponse();

// XBee will use SoftwareSerial for communications, reserve the HardwareSerial
// for debugging w/FTDI interface.
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

void setupXBee() {
    dbg("Setting up XBee...");

    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(ERROR_LED_PIN, OUTPUT);

    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);
    xbee.setSerial(ss);

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
 * Pump Switch (MAIN)
 */
uint32_t lastTransmitTime = 0;
uint32_t pumpStartTime    = 0;
uint32_t pumpStopTime     = 0;
uint32_t pumpCheckTime    = 0;
uint32_t lastStartAttemptTime = 0;
int startAttemptCount = 0;
bool pumpValuesOutChanged = false;
bool buttonLedEnabled  = false;
bool pumpRunning       = false;

// Support diagnsotic LED flashing
void flashLed(int pin, int times, int wait) {
    flashLed(false, pin, times, wait);
}

void flashLed(bool state, int pin, int times, int wait) {
    for (int i = 0; i < times; i++) {
        state ? digitalWrite(pin, LOW) : digitalWrite(pin, HIGH);
        delay(wait);
        state ? digitalWrite(pin, HIGH) : digitalWrite(pin, LOW);

        if (i + 1 < times) {
            delay(wait);
        }
    }
}

uint32_t msToMinutes(uint32_t ms) {
    return (ms / 1000) / 60;
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
 * Momentary Switch w/LED (button)
 */
Bounce debouncer = Bounce(); 

void setupButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUTTON_LED, OUTPUT);

    // start with the LED off
    digitalWrite(BUTTON_LED, LOW);

    debouncer.attach(BUTTON_PIN);
    debouncer.interval(5); // ms
}

void buttonPressed() {
    dbg("Button pressed!");

    pumpRunning ?  stopPump() : startPump();
}

void enableButtonLed() {
    // idempotent, don't check current state

    buttonLedEnabled = true;
    digitalWrite(BUTTON_LED, HIGH);
}

void disableButtonLed() {
    // idempotent, don't check current state

    buttonLedEnabled = false;
    digitalWrite(BUTTON_LED, LOW);
}

void buttonLedThinkingFlash() {
    // flash to show "thinking"
    flashLed(buttonLedEnabled, BUTTON_LED, 5, 100);
}

void buttonLedErrorFlash() {
    // flash to show error/refusal
    flashLed(buttonLedEnabled, BUTTON_LED, 5, 500);
}

void buttonLedCheckFlash() {
    // flash to show error/refusal
    flashLed(buttonLedEnabled, BUTTON_LED, 1, 250);
}

/*
 * Check how long the pump has been running and 
 * stop the pump if it exceeds MAX_PUMP_ON_MINUTES.
 */
void checkPump() {
    if (millis() - pumpCheckTime > CHECK_INTERVAL_SECONDS * 1000LU) {
        pumpCheckTime = millis();

        if (pumpRunning) {
            uint32_t pumpOnMinutes = msToMinutes(millis() - pumpStartTime);
            if (MAX_PUMP_ON_MINUTES <= pumpOnMinutes) {
                dbg("Pump run time [%lumin] exceeded max [%dmin], stopping pump", 
                        pumpOnMinutes, MAX_PUMP_ON_MINUTES);
                stopPump();
            } else {
                dbg("Pump run time [%lumin] less than max [%dmin], not stopping", 
                        pumpOnMinutes, MAX_PUMP_ON_MINUTES);
                // do nothing
            }
        }

        // regardless of pump state, always blink/flash
        buttonLedCheckFlash();
    }
}

/*
 * Start the pump, triggered either by user input (button)
 * or via request from the base station.
 */
void startPump() {
    dbg("Starting pump...");

    if (pumpRunning) {
        dbg("Pump is already running for %lumin", msToMinutes(millis() - pumpStartTime));
        // don't do anything if the pump is already running
        // TODO report back the time it has been running to base station?
        return;
    }

    // don't start the pump if it hasn't rested long enough
    // TODO make this configurable with switches on base station, allow base station
    // to transmit overrides to the default values...
    uint32_t pumpOffMinutes = msToMinutes(millis() - pumpStopTime);
    if (pumpStopTime && MIN_PUMP_OFF_MINUTES > pumpOffMinutes) {
        if (!lastStartAttemptTime || millis() - lastStartAttemptTime < PUMP_START_ATTEMPTS_WINDOW_SECONDS * 1000LU) {
            startAttemptCount++;
        } else {
            lastStartAttemptTime = millis();
            startAttemptCount = 1;
        }

        if (MIN_PUMP_START_ATTEMPTS > startAttemptCount) {
            dbg("Pump has only been off %lumin (MINIMUM: %dmin), NOT starting", pumpOffMinutes, MIN_PUMP_OFF_MINUTES);
            buttonLedErrorFlash();
            return;
        } else {
            dbg("Pump override enabled!");

            // reset the attempt counter/time
            lastStartAttemptTime = 0;
            startAttemptCount = 0;
        }
    }

    buttonLedThinkingFlash();

    pumpRunning = true;
    pumpStartTime = millis();

    enableRelay();

    // turn on the LED
    enableButtonLed();
    dbg("Pump started");
}

/*
 * Stop the pump, triggered either by user input (button)
 * or via request from the base station.
 */
void stopPump() {
    dbg("Stopping pump");

    if (!pumpRunning) {
        dbg("Pump was already stopped %lumin ago", msToMinutes(millis() - pumpStopTime));
        // don't do anything if the pump is already stopped
        // TODO anything to report back to base station?
        return;
    }

    buttonLedThinkingFlash();

    pumpRunning = false;
    pumpStopTime = millis();

    // TODO stop the pump
    disableRelay();

    // turn off the LED
    disableButtonLed();
    dbg("Pump stopped");
}

void updateCounter() {
    if (pumpRunning) {
        int maxSeconds = MAX_PUMP_ON_MINUTES * 60;

        // flash the colon (dots) every 0.5s
        bool drawColon = (millis() % 1000 < 500) ? true : false;
        counter.drawColon(drawColon);

        // setup the countdoun value
        int elapsedSeconds = maxSeconds - (int)((millis() - pumpStartTime) / 1000);

        if (0 < elapsedSeconds) {
            int elapsedMinutes = (int)(elapsedSeconds / 60);

            int elapsedMinutesTens = (int)(elapsedMinutes / 10);
            counter.writeDigitNum(0, elapsedMinutesTens, false);

            int elapsedMinutesOnes = elapsedMinutes % 10;
            counter.writeDigitNum(1, elapsedMinutesOnes, false);

            int remainderSeconds = elapsedSeconds % 60;

            int remainderSecondsTens = (int)(remainderSeconds / 10);
            counter.writeDigitNum(3, remainderSecondsTens, false);

            int remainderSecondsOnes = remainderSeconds % 10;
            counter.writeDigitNum(4, remainderSecondsOnes, false);
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
 * Check if the pumpValues changed, or FORCE_TRANSMIT_INTERVAL_SECONDS has elapsed
 * since the last pumpValues change, and transmit the pumpValues to the base station.
 */
void transmitPumpValues() {
    if (pumpValuesOutChanged || millis() - lastTransmitTime >= FORCE_TRANSMIT_INTERVAL_SECONDS * 1000LU) {
        dbg("Sending data...");
        lastTransmitTime = millis();
        
        // zbTx already has a reference to the pumpValues
        xbee.send(zbTx);

        dbg("Data sent...");
        flashLed(STATUS_LED_PIN, 1, 100);

        if (xbee.readPacket(XBEE_STATUS_WAIT_MS)) {
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

        dbg("Total transmit time: %lums", millis() - lastTransmitTime);
    }
}

void setup() {
    // hardware serial is used for FTDI debugging
    Debug.begin();

    dbg("setup() start..");

    setupUnusedPins();

    setupRelay();

    setupButton();

    setupCounter();

    setupXBee();

    dbg("setup() completed!");

    dbg("Performing initial update & transmit");

    // force initial transmitting of values
    pumpValuesOutChanged = true;

    transmitPumpValues();

    // XXX remove this once we're updating it properly
    pumpValuesOutChanged = false;
}

void loop() {

    debouncer.update();

    if (debouncer.fell()) {
        buttonPressed();
    }
    
    updateCounter();

    checkPump();

    transmitPumpValues();
}

