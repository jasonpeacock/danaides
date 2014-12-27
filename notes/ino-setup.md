# Ino Setup for Danaides using Adafruit Pro Trinket (5v & 3v)

## Adding Adafruit Pro Trinket support to Arduino IDE (v1.0.5)

Because that's where Ino will look to find build support.

1. Download the [Adafruit Pro Trinket support files](https://learn.adafruit.com/introducing-pro-trinket/setting-up-arduino-ide)
2. Edit the `Arduino/hardware/arduino/boards.txt` file and add the new board definitions from the Adafruit download.
3. Edit the new board definitions to change any `arduino:...` references to remove the `arduino:` prefix:
    e.g. `arduino:eightanaloginputs => eightanalonginputs`

## Adding Adafruit Pro Trinket support to Ino

1. Confirm the new boards are available in Ino:

        ino clean
        ino list-models

    Ino will cache `boards.txt` and other configuration files, doing `ino clean` removes the cache and uses the latest files.

2. Create an `ino.ini` configuration file in the base directory (same place as the `src/` and `lib/` dirs), add the `build`, `upload`, and `serial` sections:

        build]
        board-model = protrinket5ftdi

        [upload]
        board-model = protrinket5ftdi
        serial-port = /dev/cu.usbserial-A703X5JF

        [serial]
        serial-port = /dev/cu.usbserial-A703X5JF

    The `serial-port` value was found using the Arduino IDE to list the available Serial Ports. The `/dev/tty.usbserial-...` value didn't work, I assume that's for talking to the actual FTDI friend interface, while the `/dev/cu.usbserial-...` value is for pass-thru to talk with the Pro Trinket board.

3. A separate `ino.ini` file can be created for each project directory if needed (because using different boards, or different serial ports).
