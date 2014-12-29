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

#include <XBee.h>
#include <SoftwareSerial.h>

#include "Danaides.h"

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
#define SLEEP_PIN     10           // XBee SP (DTR)
#define SS_TX_PIN     11           // XBee RX
#define SS_RX_PIN     12           // XBee TX
#define LED           13           // pin 13 == LED_BUILTIN
#define STATUS_LED_PIN LED_BUILTIN // XBee Status LED
#define ERROR_LED_PIN  LED_BUILTIN // XBee Error LED

/*
 * Shift Register Input Constants
 */

// Width of pulse to trigger the shift register to read and latch.
#define PULSE_WIDTH_USEC 5

// Optional delay between shift register reads.
#define POLL_DELAY_MSEC 1

/*
 * Remote Sensor Constants
 */

// how often to check sensors
// NOTE: if all sensor values match previous values
//       then NO update will be transmitted.
#define CHECK_INTERVAL_SECONDS 10UL

// how often to transmit values, regardless of previous sensor values
#define FORCE_TRANSMIT_INTERVAL_SECONDS 30UL

/*
 * Setup the XBee radio w/SoftwareSerial
 */ 

XBee xbee = XBee();
SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

XBeeAddress64 addr64 = XBeeAddress64(XBEE_FAMILY_ADDRESS, BASE_STATION_ADDRESS);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));

ZBTxStatusResponse txStatus = ZBTxStatusResponse();

unsigned long lastTransmitTime = 0;
boolean lastTransmitSuccess = false;
boolean payloadChanged = true;

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

void setupShiftInput() {
    pinMode(PL_PIN, OUTPUT);
    pinMode(CE_PIN, OUTPUT);
    pinMode(CP_PIN, OUTPUT);
    pinMode(Q7_PIN, INPUT);

    digitalWrite(CP_PIN, LOW);
    digitalWrite(PL_PIN, HIGH);
}

void readShiftInputs() {
    payloadChanged = false;

    uint8_t bitValue;

    // trigger a parallel Load to latch the state of the data lines,
    digitalWrite(CE_PIN, HIGH);
    digitalWrite(PL_PIN, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(PL_PIN, HIGH);
    digitalWrite(CE_PIN, LOW);

    // loop to read each bit value from the serial out line of the SN74HC165N.
    for (int i = (TOTAL_SENSOR_INPUTS - 1); i >= 0; i--) {
        bitValue = digitalRead(Q7_PIN);

        if (TANK_1_INVERTED_FLOAT == i) {
            // invert the value so that ON/OFF
            // makes sense.
            bitValue = bitValue ? 0 : 1;
        }

        if (bitValue != payload[i]) {
            Serial.print("Value changed!!! Sensor ");
            Serial.print(i);
            Serial.print(":\t");
            Serial.print(payload[i]);
            Serial.print(" -> ");
            Serial.println(bitValue);

            payload[i] = bitValue;
            payloadChanged = true;
        }

        // pulse the Clock (rising edge shifts the next bit).
        digitalWrite(CP_PIN, HIGH);
        delayMicroseconds(PULSE_WIDTH_USEC);
        digitalWrite(CP_PIN, LOW);
    }
}

void displayPayload() {
    Serial.print("Sensor States:\r\n");

    for(int i = 0; i < TOTAL_SENSOR_INPUTS; i++)
    {
        Serial.print("  Sensor ");
        Serial.print(i);
        Serial.print(":\t");

        if(1 == payload[i]) {
            Serial.print("** ON **");
        } else {
            Serial.print("OFF");
        }

        Serial.print("\r\n");
    }

    Serial.print("\r\n");
}


void setupXBee() {
    Serial.println("Setting up XBee...");

    // XXX remove eventually, this consumes too much power
    // for battery use...
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(ERROR_LED_PIN, OUTPUT);

    pinMode(SLEEP_PIN, OUTPUT);
    digitalWrite(SLEEP_PIN, LOW);

    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);
    xbee.setSerial(ss);

    delay(XBEE_SETUP_DELAY_SECONDS * 1000);

    Serial.println("Completed XBee setup");
}

void wakeXBee() {
    //XXX fix the sleep
    //digitalWrite(SLEEP_PIN, LOW);
    // wait for radio to be ready to receive input
    // XXX use CTS to detect when ready?
    delay(50);
}

void sleepXBee() {
    //XXX fix the sleep
    //digitalWrite(SLEEP_PIN, HIGH);
    // XXX investigate using SLEEP_PIN as INPUT instead, less power?
    // http://www.fiz-ix.com/2012/11/low-power-xbee-sleep-mode-with-arduino-and-pin-hibernation/
}

/*
 * Check if the payload changed, or FORCE_TRANSMIT_INTERVAL_SECONDS has elapsed,
 * and transmit the payload to the base station.
 */
void transmitPayload() {
    if (payloadChanged || (unsigned long)(millis() - lastTransmitTime) >= FORCE_TRANSMIT_INTERVAL_SECONDS * 1000) {
        lastTransmitTime = millis();

        wakeXBee();

        Serial.println("Sending data...");
        // send the data! zbTx already has a reference to the payload
        xbee.send(zbTx);

        delay(XBEE_TRANSMIT_DELAY_SECONDS);

        Serial.println("Data sent!");
        flashLed(STATUS_LED_PIN, 1, 100);

        if (xbee.readPacket(STATUS_WAIT_MS)) {
            Serial.print("RESPONSE_API_ID=");
            Serial.print(xbee.getResponse().getApiId());
            Serial.print(" i.e. ");

            if (ZB_TX_STATUS_RESPONSE == xbee.getResponse().getApiId()) {
                Serial.println("ZB_TX_STATUS_RESPONSE");
                xbee.getResponse().getZBTxStatusResponse(txStatus);

                if (SUCCESS == txStatus.getDeliveryStatus()) {
                    lastTransmitSuccess = true;
                    Serial.println("Success!");
                    flashLed(STATUS_LED_PIN, 5, 50);
                } else {
                    lastTransmitSuccess = false;
                    Serial.println("Failure :(");
                    flashLed(ERROR_LED_PIN, 3, 500);
                }
            }
        } else if (xbee.getResponse().isError()) {
            lastTransmitSuccess = false;
            Serial.print("Error reading packet. Error code: ");
            Serial.println(xbee.getResponse().getErrorCode());
        } else {
            lastTransmitSuccess = false;
            Serial.println("Local XBee didn't provide a timely TX status response (should not happen)");
            flashLed(ERROR_LED_PIN, 2, 50);
        }

        sleepXBee();
    }
}

void checkSensors() {
    Serial.println("Checking values...");

    readShiftInputs();

    displayPayload();
}

void sleep() {
    Serial.print("Waiting ");
    Serial.print(CHECK_INTERVAL_SECONDS);
    Serial.println("s before doing it all again...");
    // XXX go to sleep here to save battery power
    delay(CHECK_INTERVAL_SECONDS * 1000);
}

void setup() {
    // hardware serial is used for FTDI debugging
    Serial.begin(9600);
    Serial.println("setup() start..");

    setupShiftInput();
    setupXBee();

    // get the initial values at startup
    readShiftInputs();
    displayPayload();

    // force initial transmitting of values
    payloadChanged = true;
    transmitPayload();

    Serial.println("setup() completed!");
}

void loop() {
    checkSensors();

    transmitPayload();

    sleep();
}

