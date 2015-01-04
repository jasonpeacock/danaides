// local
#include "Data.h"

/*
 * Private
 */

void Data::_init(uint16_t address, uint8_t* data, uint8_t size) {
    _address = address;
    _size = size;

    if (_initialized) {
        delete [] _data;
    }

    // make a copy of the data
    _data = new uint8_t[_size];
    for (uint8_t i = 0; i < _size; i++) {
        _data[i] = data[i];
    }

    _initialized = true;
}

/*
 * Public
 */

Data::Data() : _initialized(false), 
               _address(0), 
               _data(NULL),
               _size(0) {
}

Data::Data(uint16_t address, uint8_t *data, uint8_t size) : _initialized(false) {
    _init(address, data, size);
}

Data::~Data() {
    if (_initialized) {
        delete [] _data;
    }
}

void Data::set(uint16_t address, uint8_t *data, uint8_t size) {
    _init(address, data, size);
}

uint8_t* Data::getData() {
    return _data;
}

uint8_t Data::getSize() {
    return _size;
}

uint16_t Data::getAddress() {
    return _address;
}

bool Data::hasAddress() {
    return _address;
}

