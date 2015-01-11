#ifndef Message_h
#define Message_h

// system
#include <Arduino.h>

// third-party
#include "Adafruit_LEDBackpack.h"

/*
 * Constants
 */

#define MESSAGE_ADDRESS_ALPHA_1 0x70
#define MESSAGE_ADDRESS_ALPHA_2 0x71
#define MESSAGE_ALPHA_CHAR_SIZE 4
#define MESSAGE_TOTAL_CHAR_SIZE 8

#define MESSAGE_MAX_SIZE 250

#define MESSAGE_SCROLL_DELAY_MILLIS 200UL

class Message {
    private:
        Adafruit_AlphaNum4 _alpha_1;
        Adafruit_AlphaNum4 _alpha_2;

        char _message[MESSAGE_MAX_SIZE];
        uint8_t _messageSize;

        uint32_t _lastScrollTime;
        uint8_t _scrollPosition;
        char _scrollBuffer[MESSAGE_TOTAL_CHAR_SIZE];

    public:
        Message();
        ~Message();

        void setup();
        void check();

        void reset();
        void setMessage(char* message);
};

#endif //Message_h
