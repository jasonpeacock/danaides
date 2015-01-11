// system
#include <Arduino.h>

// third-party
#include "Adafruit_LEDBackpack.h"

// local
#include "Message.h"

/*
 * Private
 */

/*
 * Public
 */

Message::Message() : 
    _alpha_1(Adafruit_AlphaNum4()), 
    _alpha_2(Adafruit_AlphaNum4()),
    _address_1(0),
    _address_2(0),
    _messageSize(0),
    _lastScrollTime(0UL),
    _scrollPosition(0) {
}

Message::Message(uint8_t address_1, uint8_t address_2) : 
    _alpha_1(Adafruit_AlphaNum4()), 
    _alpha_2(Adafruit_AlphaNum4()),
    _address_1(address_1),
    _address_2(address_2),
    _messageSize(0),
    _lastScrollTime(0UL),
    _scrollPosition(0) {
}

Message::~Message() {
}

void Message::setup() {
    _alpha_1.begin(_address_1);
    _alpha_2.begin(_address_2);

    reset();
}

void Message::reset() {
    _alpha_1.clear();
    _alpha_2.clear();

    _alpha_1.writeDisplay();
    _alpha_2.writeDisplay();

    // init the buffer
    for (uint8_t i = 0; i < MESSAGE_TOTAL_CHAR_SIZE; i++) {
        _scrollBuffer[i] = ' ';
    }

    // reset the vars
    _lastScrollTime = 0UL;
    _scrollPosition = 0;
}

void Message::setMessage(char* message) {
    // copy the message
    strncpy(_message, message, MESSAGE_MAX_SIZE - 1);
    // just in case message was too large
    _message[MESSAGE_MAX_SIZE - 1] = '\0';

    _messageSize = strlen(_message);

    reset();
}

void Message::check() {
    if (_messageSize + MESSAGE_TOTAL_CHAR_SIZE <= _scrollPosition) {
        // everything has been scrolled, nothing to do
        return;
    }

    if (millis() - _lastScrollTime > MESSAGE_SCROLL_DELAY_MILLIS) {
        // scroll the message off the display by including extra empty chars
        char c = _message[_scrollPosition];
        if (_messageSize <= _scrollPosition) {
            c = ' ';
        }

        for (uint8_t i = 0; i < MESSAGE_TOTAL_CHAR_SIZE; i++) {
            if (i == MESSAGE_TOTAL_CHAR_SIZE - 1) {
                _scrollBuffer[i] = c;
            } else {
                _scrollBuffer[i] = _scrollBuffer[i + 1];
            }

            if (MESSAGE_ALPHA_CHAR_SIZE > i) {
                // first 4 chars
                _alpha_1.writeDigitAscii(i, _scrollBuffer[i]);
            } else {
                // last 4 chars
                _alpha_2.writeDigitAscii(i - MESSAGE_ALPHA_CHAR_SIZE, _scrollBuffer[i]);
            }
        }

        _alpha_1.writeDisplay();
        _alpha_2.writeDisplay();

        _lastScrollTime = millis();
        _scrollPosition++;
    }
}


