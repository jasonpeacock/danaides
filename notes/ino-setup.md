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

# Reporting memory size of builds

The Trinket Pro (and Arduino Uno) only have 2k of RAM, and ~28k of flash. If there is not enough memory then bad things will happen. Do a static (good-enough) analysis of the build to see how much memory is being used:

From within a project directory, run the following:

`ino build && /Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-size .build/protrinket5ftdi/firmware.elf`

The output will be similar to:

     text     data     bss     dec     hex filename
    17794      172     713   18679    48f7 .build/protrinket5ftdi/firmware.elf

Where the application (flash) usage is `text` and the memory (SRAM) usage is `data + bss`.

## How to save memory

Don't use `Dbg`, alas. It is very nice and convenient, but it also doesn't support the F() macro which stores string literals (`"foo bar"`) in flash instead of SRAM. This is a huge savings, all print statements should be composed by doing `Serial.println(F("foo bar));` to avoid wasting SRAM.

