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

//XXX update to correct values! (???)
// how often to transmit values
#define PUMP_SWITCH_TRANSMIT_INTERVAL_SECONDS 10UL

//XXX adjust these to actual values 
#define PUMP_DEFAULT_MAX_ON_MINUTES   2UL // 30
#define PUMP_LONG_MAX_ON_MINUTES      4UL // 60
#define PUMP_DEFAULT_MIN_ON_MINUTES   1UL // 10 (?)
#define PUMP_DEFAULT_MIN_OFF_MINUTES  2UL // 30
#define PUMP_LONG_MIN_OFF_MINUTES     4UL // 60

#define PUMP_START_ATTEMPTS_WINDOW_SECONDS 60UL
#define PUMP_MIN_START_ATTEMPTS 3

#define EVALUATE_TANK_SENSOR_INTERVAL_MINUTES 1UL

//XXX adjust these to actual values 
#define PUMP_SWITCH_RECEIVE_ALARM_DELAY_MINUTES 1UL
#define REMOTE_SENSOR_RECEIVE_ALARM_DELAY_MINUTES 2UL

void freeRam();

#endif //Danaides_h
