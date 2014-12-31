# XBee Radio Configuration and Notes

3 XBee Pro S2B (Series 2) 63mw radios are used to communicate between the Base Station (Coordinator), Remote Sensor (End Unit), and Pump Switch (End Unit).

They are mounted on Adafruit XBee Adapter Boards (FTDI, not USB).

The configuration differs slightly between each radio based on its role and power supply type.

# Base Station Coordinator

Has "B" written on radio w/marker.

## Non-default or Device-specific settings

* Product Family: XBP24BZ7
* Function Set: ZigBee Coordinator API
* ID: DECAF
* NI: BaseStation Coord
* SH: 13A200
* SL: 40C59926
* D7: 0 (Disabled, was CTS)
* AP: 2
* SP: AFO (Max value, 2800)
* D0: 0 (Disabled, was Commissioning)
* SN: FFFF (Max value, 65,535)
* VR: 21A7 (Firmware Version)
* HV: 1E56 (Hardware Version)

## Notes

Runs on mains power.

Use SP=AFO (2800 * 10ms = 28s) and SN=FFF (65,535 * 3 * 28s = ~63days) to set the poll timeout to a very large timeout and avoid dropping sleeping End Unit devices which aren't talking to the Coordinator frequently enough. Otherwise, when an End Unit sleeps the Coordinator forgets it and the next wake period the End Unit must re-join the network (which takes 5-10s), which is very expensive for battery power. [More Info](http://www.digi.com/support/forum/2059/xbee-pro-s2b-wake-association-time)

# Remote Sensor End Unit

Has "R" written on radio w/marker.

## Non-default or Device-specific settings

* Product Family: XBP24BZ7
* Function Set: ZigBee End Device API
* ID: DECAF
* NI: RemoteSensor EndUnit
* SH: 13A200
* SL: 40C59899
* AP: 2
* SM: 1 (Sleep Mode = Pin Hibernate)
* D0: 0 (Disabled, was Commissioning)
* D5: 0 (Disabled, was Associated indicator) ***TODO***
* P0: 0 (Disabled, was RSSI PWM Output) ***TODO***
* PR: Set internal PullUp for unused pins? ***TODO***
* VR: 29A7 (Firmware Version)
* HV: 1E46 (Hardware Version)

## Notes

Runs on battery power.

Uses CTS (default) and RTS (enabled) Flow Control to determine when radio is available for transmitting after waking from sleep, and when radio is finished before putting back to sleep.

Uses Pin Sleep for lowest power consumption, and disables all unnecessary pins/LEDs.

# Pump Switch End Unit

Has "P" written on radio w/marker.

## Non-default or Device-specific settings

* Product Family: XBP24BZ7
* Function Set: ZigBee End Device API
* ID: DECAF
* NI: PumpSwitch EndUnit
* SH: 13A200
* SL: 40C31683
* D7: 0 (Disabled, was CTS Flow Control)
* AP: 2
* SM: 1 (Sleep Mode = Pin Hibernate)
* D0: 0 (Disabled, was Commissioning)
* VR: 29A7 (Firmware Version)
* HV: 1E46 (Hardware Version)

## Notes

Runs on mains power.

Uses Pin Sleep grounded to force End Unit to never sleep.

