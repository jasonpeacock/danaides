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

#include <XBee.h>
#include <SoftwareSerial.h>

/*
 * Constants
 */

#define STATUS_LED 13
#define ERROR_LED  13

#define SS_RX_PIN  12
#define SS_TX_PIN  11

// how long to wait for a status response after sending a message
#define STATUS_WAIT_MS 500

SoftwareSerial ss(SS_RX_PIN, SS_TX_PIN);

XBee xbee = XBee();

// Address of receiving (Base Station) radio
XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x40C59926);

uint8_t payload[] = {0, 1};

ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

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

void setup() {
    // hardware serial is being used for FTDI debugging
    Serial.begin(9600);
    Serial.println("setup() start..");

    pinMode(STATUS_LED, OUTPUT);
    pinMode(ERROR_LED, OUTPUT);

    // software serial is used for XBee communications
    pinMode(SS_RX_PIN, INPUT);
    pinMode(SS_TX_PIN, OUTPUT);
    ss.begin(9600);
    xbee.setSerial(ss);

    Serial.println("setup() completed!");
}

void loop() {
    Serial.println("About to send values...");

    // flip-flop the values for testing...
    if (0 == payload[0]) {
        payload[0] = 1;
        payload[1] = 0;
    } else {
        payload[0] = 0;
        payload[1] = 1;
    }

    Serial.print("Sending values: ");
    Serial.print(payload[0]);
    Serial.print(" ");
    Serial.println(payload[1]);

    // send the data!
    xbee.send(zbTx);

    Serial.println("Data sent!");
    flashLed(STATUS_LED, 1, 100);

    Serial.print("Waiting up to ");
    Serial.print(STATUS_WAIT_MS);
    Serial.println("ms for a response");

    if (xbee.readPacket(STATUS_WAIT_MS)) {
        Serial.print("RESPONSE_API_ID=");
        Serial.print(xbee.getResponse().getApiId());
        Serial.print(" i.e. ");

        if (ZB_TX_STATUS_RESPONSE == xbee.getResponse().getApiId()) {
            Serial.println("ZB_TX_STATUS_RESPONSE");
            xbee.getResponse().getZBTxStatusResponse(txStatus);

            if (SUCCESS == txStatus.getDeliveryStatus()) {
                Serial.println("Success!");
                flashLed(STATUS_LED, 5, 50);
            } else {
                Serial.println("Failure :(");
                flashLed(ERROR_LED, 3, 500);
            }
        }
    } else if (xbee.getResponse().isError()) {
        Serial.print("Error reading packet. Error code: ");
        Serial.println(xbee.getResponse().getErrorCode());
    } else {
        Serial.println("Local XBee didn't provide a timely TX status response (should not happen)");
        flashLed(ERROR_LED, 2, 50);
    }

    Serial.println("Waiting 10s before doing it all again...");
    delay(10000);
}

