#ifndef Danaides_h
#define Danaides_h

// system
#include <Arduino.h>

//XXX update to correct values! (10min/30min?)
// how often to check sensors
// NOTE: if all sensor values match previous values
//       then NO update will be transmitted.
#define REMOTE_SENSOR_CHECK_INTERVAL_SECONDS 10UL
// how often to transmit values, regardless of previous sensor values
#define REMOTE_SENSOR_FORCE_TRANSMIT_INTERVAL_SECONDS 30UL

// How long WAN should wait for data when receiving
// before giving up.
#define REMOTE_SENSOR_RECEIVE_TIMEOUT_MS 100

// how often to transmit values
#define PUMP_SWITCH_TRANSMIT_INTERVAL_SECONDS 30UL

#define PUMP_DEFAULT_MAX_ON_MINUTES   45UL
#define PUMP_LONG_MAX_ON_MINUTES      75UL
#define PUMP_DEFAULT_MIN_ON_MINUTES   10UL
#define PUMP_DEFAULT_MIN_OFF_MINUTES  45UL
#define PUMP_LONG_MIN_OFF_MINUTES     75UL

#define PUMP_START_ATTEMPTS_WINDOW_SECONDS 60UL
#define PUMP_MIN_START_ATTEMPTS 3

#define EVALUATE_TANK_SENSOR_INTERVAL_MINUTES 1UL

//XXX update to correct values (5min/1hour?)
#define PUMP_SWITCH_RECEIVE_ALARM_DELAY_MINUTES 5UL
#define REMOTE_SENSOR_RECEIVE_ALARM_DELAY_MINUTES 10UL

void freeRam(bool enable = false);

#endif //Danaides_h
