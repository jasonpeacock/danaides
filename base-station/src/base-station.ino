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

/*
 * Constants
 */

/*
 * Pins
 */

#define BUTTON_PIN     3           // Momentary switch
#define BUTTON_LED     4           // Momentary switch LED (blue)
#define EVALUATE_PIN   5           // Switch to allow BaseStation to manager PumpSwitch
#define LATE_PUMP_LED  6           // Late PumpSwitch LED (green)
#define NO_PIN_7       7           // no pin 7 on Trinket Pro boards
#define LATE_TANK_LED  8           // Late TankSensors LED (green)
#define VALVE_1_LED    9           // Valve 1 Status LED (red)
#define VALVE_2_LED   10           // Valve 2 Status LED (red)
#define VALVE_3_LED   11           // Valve 3 Status LED (red)
#define VALVE_4_LED   12           // Valve 4 Status LED (red)
#define STATUS_LED    13           // XBee Status LED
#define VALVE_5_LED   14           // Valve 5 Status LED (red) (Analog 0)
#define EVALUATE_LED  15           // Evaluate Enabled LED (blue) (Analog 1)
#define SS_TX_PIN     16           // XBee RX   (Analog 2)
#define SS_RX_PIN     17           // XBee TX   (Analog 3)
#define I2C_DATA_PIN  18           // I2C Data  (Analog 4)
#define I2C_CLOCK_PIN 19           // I2C Clock (Analog 5)

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
PumpSwitch pumpSwitch = PumpSwitch(false, BUTTON_PIN, BUTTON_LED, enablePump, disablePump);
TankSensors tankSensors = TankSensors();

uint32_t lastPumpSwitchValuesReceiveTime = 0UL;
uint32_t lastPumpSwitchSettingsReceiveTime = 0UL;
uint32_t lastRemoteSensorReceiveTime = 0UL;

bool isPumpSwitchReceiveLate() {
    bool late = false;
    if (millis() - lastPumpSwitchSettingsReceiveTime > PUMP_SWITCH_RECEIVE_ALARM_DELAY_MINUTES * 60UL * 1000UL) {
        late = true;
    }

    if (millis() - lastPumpSwitchValuesReceiveTime > PUMP_SWITCH_RECEIVE_ALARM_DELAY_MINUTES * 60UL * 1000UL) {
        late = true;
    }

    return late;
}

bool isRemoteSensorReceiveLate() {
    bool late = false;
    if (millis() - lastRemoteSensorReceiveTime > REMOTE_SENSOR_RECEIVE_ALARM_DELAY_MINUTES * 60UL * 1000UL) {
        late = true;
    }

    return late;
}

// Evaluate Switch
Bounce evaluateSwitch = Bounce();
bool evaluateEnabled = true;
LED evaluateLed = LED(EVALUATE_LED);
void setupEvaluate() {
    evaluateLed.setup();

    pinMode(EVALUATE_PIN, INPUT_PULLUP);
    evaluateSwitch.attach(EVALUATE_PIN);
    evaluateSwitch.interval(5); // ms

    evaluateSwitch.update();
    evaluateEnabled = evaluateSwitch.read();
}

void evaluateSwitchCheck() {
    if (evaluateSwitch.update()) {
        // switch was switched
        evaluateEnabled = evaluateSwitch.read();

        Serial.print(F("Evaluate switch updated: "));
        Serial.println(evaluateEnabled);
    }

    if (evaluateEnabled && 
            (!tankSensors.ready() || isRemoteSensorReceiveLate() ||
             !pumpSwitch.ready() || isPumpSwitchReceiveLate())) {
        evaluateLed.flashing(true);
    } else if (evaluateEnabled) {
        evaluateLed.flashing(false);
        evaluateLed.on();
    } else {
        evaluateLed.off();
    }

    evaluateLed.check();
}

Message message = Message(0x70, 0x71);
Counter counter = Counter(0x72);
Bargraph bar_1 = Bargraph(0x73, tankSensors.getNumFloatsPerTank());
Bargraph bar_2 = Bargraph(0x74, tankSensors.getNumFloatsPerTank());
Bargraph bar_3 = Bargraph(0x75, tankSensors.getNumFloatsPerTank());
LED latePumpSwitchLed  = LED(LATE_PUMP_LED);
LED lateTankSensorsLed = LED(LATE_TANK_LED);
LED valveLed_1 = LED(VALVE_1_LED);
LED valveLed_2 = LED(VALVE_2_LED);
LED valveLed_3 = LED(VALVE_3_LED);
LED valveLed_4 = LED(VALVE_4_LED);
LED valveLed_5 = LED(VALVE_5_LED);

Display display = Display(message, 
                          counter, 
                          bar_1, 
                          bar_2, 
                          bar_3, 
                          latePumpSwitchLed, 
                          lateTankSensorsLed,
                          valveLed_1,
                          valveLed_2,
                          valveLed_3,
                          valveLed_4,
                          valveLed_5);

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
    Data data = Data();
    if (wan.receive(data)) {
        Serial.println(F("Received data..."));
        Serial.flush();

        if (wan.isBaseStationAddress(data.getAddress())) {
            Serial.println(F("New data from Base Station"));
            // we're the base station, this would be weird
        } else if (wan.isRemoteSensorAddress(data.getAddress())) {
            Serial.println(F("New data from Remote Sensor"));
            if (tankSensors.getNumSensors() == data.getSize()) {
                tankSensors.update(data);
                lastRemoteSensorReceiveTime = millis();
                Serial.println(F("Updated Tank Sensor values"));
            }
        } else if (wan.isPumpSwitchAddress(data.getAddress())) {
            Serial.println(F("New data from Pump Switch"));
            // update the local PumpSwitch object, the remote switch is always
            // the authority for the values & settings
            if (pumpSwitch.getNumSettings() == data.getSize()) {
                pumpSwitch.updateSettings(data.getData(), data.getSize());
                lastPumpSwitchSettingsReceiveTime = millis();
                Serial.println(F("Updated Pump Switch settings"));
            } else if (pumpSwitch.getNumValues() == data.getSize()) {
                pumpSwitch.updateValues(data.getData(), data.getSize());
                lastPumpSwitchValuesReceiveTime = millis();
                Serial.println(F("Updated Pump Switch values"));
            }
        }

        for (uint8_t i = 0; i < data.getSize(); i++) {
            Serial.print(i);
            Serial.print(F(":\t"));
            Serial.println(data.getData()[i]);
        }

        freeRam();
    }
}

void transmit() {
    uint32_t start = millis();

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
    if (!lastTankSensorCheckTime || millis() - lastTankSensorCheckTime > EVALUATE_TANK_SENSOR_INTERVAL_MINUTES * 60UL * 1000UL) {
        lastTankSensorCheckTime = millis();

        if (!tankSensors.ready() || isRemoteSensorReceiveLate()) {
            Serial.println(F("TankSensors not updated yet or are late, skipping evaluation"));
            // don't try to manage the pump, may conflict with user button press of pump
            return;
        }

        if (!pumpSwitch.ready() || isPumpSwitchReceiveLate()) {
            Serial.println(F("PumpSwitch not updated yet or is late, skipping evaluation"));
            // don't try to manage the pump, may conflict with user button press of pump
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

    pumpSwitch.setup();

    display.setup();

    setupWAN();

    setupEvaluate();

    Serial.println(F("setup() completed!"));

    // say something
    display.scroll("HELLO");

    freeRam();
}

void loop() {
    pumpSwitch.check();
    wan.check();

    display.check(pumpSwitch, isPumpSwitchReceiveLate(), tankSensors, isRemoteSensorReceiveLate());

    evaluateSwitchCheck();

    receive();

    evaluateTankSensors();
}

