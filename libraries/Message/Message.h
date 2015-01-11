#ifndef Message_h
#define Message_h

// system
#include <Arduino.h>

// third-party
#include "Adafruit_LEDBackpack.h"

/*
 * Constants
 */

#define MESSAGE_ALPHA_CHAR_SIZE 4
#define MESSAGE_TOTAL_CHAR_SIZE 8

//XXX make this smaller?
#define MESSAGE_MAX_SIZE 100

#define MESSAGE_SCROLL_DELAY_MILLIS 200UL

class Message {
    private:
        Adafruit_AlphaNum4 _alpha_1;
        Adafruit_AlphaNum4 _alpha_2;

        uint8_t _address_1;
        uint8_t _address_2;

        char _message[MESSAGE_MAX_SIZE];
        uint8_t _messageSize;

        uint32_t _lastScrollTime;
        uint8_t _scrollPosition;
        char _scrollBuffer[MESSAGE_TOTAL_CHAR_SIZE];

    public:
        Message();
        Message(uint8_t address_1, uint8_t address_2);
        ~Message();

        void setup();
        void check();

        void reset();
        void setMessage(char* message);
};

#endif //Message_h
