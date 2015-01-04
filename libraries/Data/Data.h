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

        uint32_t _address;
        uint8_t*  _data;
        uint8_t   _size;

        void _init(uint32_t address, uint8_t* data, uint8_t size);

    public:
        Data();
        Data(uint32_t address, uint8_t* data, uint8_t size);
        ~Data();

        void set(uint32_t address, uint8_t* data, uint8_t size);

        uint32_t getAddress();
        uint8_t* getData();
        uint8_t  getSize();

        bool     hasAddress();
};

#endif //Data_h
