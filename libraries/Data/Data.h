#ifndef Data_h
#define Data_h

// system
#include <Arduino.h>

/*
 * Constants
 */

class Data {
    private:
        bool _initialized;

        uint16_t  _address;
        uint8_t*  _data;
        uint8_t   _size;

        void _init(uint16_t address, uint8_t* data, uint8_t size);

    public:
        Data();
        Data(uint16_t address, uint8_t* data, uint8_t size);
        ~Data();

        void set(uint16_t address, uint8_t* data, uint8_t size);

        uint16_t getAddress();
        uint8_t* getData();
        uint8_t  getSize();

        bool     hasAddress();
};

#endif //Data_h
