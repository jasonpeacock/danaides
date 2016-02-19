#ifndef Danaides_h
#define Danaides_h

// system
#include <Arduino.h>

// How often to check sensors
// NOTE: if all sensor values match previous values
//       then NO update will be transmitted.
#define REMOTE_SENSOR_CHECK_INTERVAL_SECONDS 300UL // 5 minutes
// How often to transmit values, regardless of previous sensor values
#define REMOTE_SENSOR_FORCE_TRANSMIT_INTERVAL_SECONDS 1800UL // 30 minutes

// How long WAN should wait for data when receiving
// before giving up.
#define REMOTE_SENSOR_RECEIVE_TIMEOUT_MS 0UL // 0 seconds - don't wait for timeout max, just check once

// How often to transmit values
#define PUMP_SWITCH_TRANSMIT_INTERVAL_SECONDS 15UL

#define PUMP_DEFAULT_MAX_ON_MINUTES   45UL
#define PUMP_LONG_MAX_ON_MINUTES      75UL
#define PUMP_DEFAULT_MIN_ON_MINUTES   10UL
#define PUMP_DEFAULT_MIN_OFF_MINUTES  45UL
#define PUMP_LONG_MIN_OFF_MINUTES     75UL

#define EVALUATE_TANK_SENSOR_INTERVAL_MINUTES 1UL

#define PUMP_SWITCH_RECEIVE_ALARM_DELAY_MINUTES 1UL
#define REMOTE_SENSOR_RECEIVE_ALARM_DELAY_MINUTES 35UL

void freeRam(bool enable = false);

#endif //Danaides_h
