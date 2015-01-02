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

// how often to check pump state
#define CHECK_INTERVAL_SECONDS 5

// how often to transmit values
#define TRANSMIT_INTERVAL_SECONDS 60

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
ZBTxRequest zbTx = ZBTxRequest(addr64, pumpValues, sizeof(pumpValues));

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
int maxPumpOnMinutes  = DEFAULT_MAX_PUMP_ON_MINUTES;
int minPumpOffMinutes = DEFAULT_MIN_PUMP_OFF_MINUTES;

bool buttonLedEnabled  = false;
bool pumpRunning       = false;

// Support diagnsotic LED flashing
void flashLed(int pin, int times, int wait) {
    flashLed(false, pin, times, wait);
}

// flash/blink the LED, based on current button LED state
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
 * stop the pump if it exceeds the limit.
 */
void checkPump() {
    if (millis() - pumpCheckTime > CHECK_INTERVAL_SECONDS * 1000LU) {
        pumpCheckTime = millis();

        if (pumpRunning) {
            uint32_t pumpOnMinutes = msToMinutes(millis() - pumpStartTime);
            if (maxPumpOnMinutes <= pumpOnMinutes) {
                dbg("Pump run time [%lumin] exceeded max [%dmin], stopping pump", 
                        pumpOnMinutes, maxPumpOnMinutes);
                stopPump();
            } else {
                dbg("Pump run time [%lumin] less than max [%dmin], not stopping", 
                        pumpOnMinutes, maxPumpOnMinutes);
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
        return;
    }

    // don't start the pump if it hasn't rested long enough
    // TODO make this configurable with switches on base station, allow base station
    // to transmit overrides to the default values...
    uint32_t pumpOffMinutes = msToMinutes(millis() - pumpStopTime);
    if (pumpStopTime && minPumpOffMinutes > pumpOffMinutes) {
        if (!lastStartAttemptTime || millis() - lastStartAttemptTime < PUMP_START_ATTEMPTS_WINDOW_SECONDS * 1000LU) {
            startAttemptCount++;
        } else {
            lastStartAttemptTime = millis();
            startAttemptCount = 1;
        }

        if (MIN_PUMP_START_ATTEMPTS > startAttemptCount) {
            dbg("Pump has only been off %lumin (MINIMUM: %dmin), NOT starting", pumpOffMinutes, minPumpOffMinutes);
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
    pumpValues[PUMP_STATE] = pumpRunning;
    pumpValues[PUMP_MINUTES] = 0;
    pumpValues[PUMP_SECONDS] = 0;

    pumpStartTime = millis();

    // actually start the pump!
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
        return;
    }

    buttonLedThinkingFlash();

    pumpRunning = false;
    pumpValues[PUMP_STATE] = pumpRunning;
    pumpValues[PUMP_MINUTES] = 0;
    pumpValues[PUMP_SECONDS] = 0;

    pumpStopTime = millis();

    // actually stop the pump!
    disableRelay();

    // turn off the LED
    disableButtonLed();
    dbg("Pump stopped");
}

/*
 * Calculate the elapsed time since the pump started/stopped
 * as Days, Hours, Minutes, Seconds and update the pumpValues,
 * then update the 7-segment display if the pump is running with
 * the elapsed MIN:SEC.
 *
 * The Days & Hours are calculated for the pumpValues because
 * the pump may be off for multiple hours/days and we need to inform
 * the base station.
 *
 * The 7-segment display always uses elapsedMinutes and never tries
 * to show hours b/c the pump should never be on continously longer
 * then 60-90minutes.
 */
void updateCounterAndPumpValues() {
    pumpValues[PUMP_STATE] = pumpRunning;

    if (pumpRunning) {
        uint32_t maxSeconds = maxPumpOnMinutes * 60;

        // flash the colon every 0.5s
        bool drawColon = (millis() % 1000 < 500) ? true : false;
        counter.drawColon(drawColon);

        // setup the countdoun value, use not-unsigned int so that we know
        // if it's gone negative (b/c checkPump cycle doesn't align perfectly
        // with this counter updater).
        int32_t elapsedSeconds = maxSeconds - (uint32_t)((millis() - pumpStartTime) / 1000LU);

        if (0 < elapsedSeconds) {
            uint32_t elapsedMinutes = (uint32_t)(elapsedSeconds / 60);
            uint32_t elapsedHours   = (uint32_t)(elapsedMinutes / 60);
            uint8_t  elapsedDays    = (uint32_t)(elapsedHours / 24);

            uint32_t remainderHours   = elapsedHours % 24;
            uint32_t remainderMinutes = elapsedMinutes % 60;
            uint32_t remainderSeconds = elapsedSeconds % 60;

            pumpValues[PUMP_DAYS]    = elapsedDays;
            pumpValues[PUMP_HOURS]   = remainderHours;
            pumpValues[PUMP_MINUTES] = remainderMinutes;
            pumpValues[PUMP_SECONDS] = remainderSeconds;

            counter.writeDigitNum(0, elapsedMinutes / 10, false);
            counter.writeDigitNum(1, elapsedMinutes % 10, false);
            counter.writeDigitNum(3, remainderSeconds / 10, false);
            counter.writeDigitNum(4, remainderSeconds % 10, false);
        } else {
            pumpValues[PUMP_DAYS]    = 0;
            pumpValues[PUMP_HOURS]   = 0;
            pumpValues[PUMP_MINUTES] = 0;
            pumpValues[PUMP_SECONDS] = 0;

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
        // pump not running, still update the pump values
        uint32_t elapsedSeconds = (uint32_t)((millis() - pumpStopTime) / 1000LU);
        uint32_t elapsedMinutes = (uint32_t)(elapsedSeconds / 60);
        uint32_t elapsedHours   = (uint32_t)(elapsedMinutes / 60);
        uint8_t  elapsedDays    = (uint32_t)(elapsedHours / 24);

        uint8_t remainderHours   = elapsedHours % 24;
        uint8_t remainderMinutes = elapsedMinutes % 60;
        uint8_t remainderSeconds = elapsedSeconds % 60;

        pumpValues[PUMP_DAYS]    = elapsedDays;
        pumpValues[PUMP_HOURS]   = remainderHours;
        pumpValues[PUMP_MINUTES] = remainderMinutes;
        pumpValues[PUMP_SECONDS] = remainderSeconds;

        // clear the display
        counter.clear();
    }

    counter.writeDisplay();
}

void setupPumpValues() {
    pumpValues[PUMP_STATE]   = 0;
    pumpValues[PUMP_DAYS]    = 0;
    pumpValues[PUMP_HOURS]   = 0;
    pumpValues[PUMP_MINUTES] = 0;
    pumpValues[PUMP_SECONDS] = 0;
}

/*
 * Receive pumpValues from the base station
 */
void receivePumpValues() {
    // TODO receive data from XBee radio
}

/*
 * Transmit pumpValues to the base station.
 */
void transmitPumpValues() {
    if (!lastTransmitTime || millis() - lastTransmitTime >= TRANSMIT_INTERVAL_SECONDS * 1000LU) {
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

    setupPumpValues();

    setupRelay();

    setupButton();

    setupCounter();

    setupXBee();

    dbg("setup() completed!");

    dbg("Performing initial update & transmit");

    transmitPumpValues();
}

void loop() {

    debouncer.update();
    if (debouncer.fell()) {
        buttonPressed();
    }
    
    checkPump();

    updateCounterAndPumpValues();

    transmitPumpValues();
}

