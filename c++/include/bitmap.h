#pragma once

#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <iostream>
#include <math.h>
#include <stdint.h>

class Bitmap;
class BitRow;

// ----------------------------------------------------------------------------------------------------
// BitPointer class
// ----------------------------------------------------------------------------------------------------

class BitPointer {
public:
    BitPointer(int x, int y, Bitmap* owner) : x(x), y(y), owner(owner) {}
    ~BitPointer() {}

    int x, y;
    Bitmap* owner;

    void operator=(bool other);
    operator bool() const;
};

// ----------------------------------------------------------------------------------------------------
// BitRow class
// ----------------------------------------------------------------------------------------------------

class BitRow {
public:
    BitRow(int y, Bitmap* owner) : y(y), owner(owner) {}

    int y;
    Bitmap* owner;

    BitPointer operator[](int x);
};

// ----------------------------------------------------------------------------------------------------
// Bitmap class
// ----------------------------------------------------------------------------------------------------

class Bitmap {
private:
    int _dataSize;
    uint8_t* _data = nullptr;

public:
    Bitmap(int width, int height, bool defaultValue = false);
    ~Bitmap();

    int width, height;

    bool get(int x, int y);
    void set(int x, int y, bool value);

    BitRow operator[](int y);
};

// ----------------------------------------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------------------------------------

// --------------------------------------------------
// BitPointer
// --------------------------------------------------

void BitPointer::operator=(bool other) {
    owner->set(x, y, other);
}

BitPointer::operator bool() const {
    return owner->get(x, y);
}

// --------------------------------------------------
// BitRow
// --------------------------------------------------

BitPointer BitRow::operator[](int x) {
    if (x >= owner->width)
        throw std::invalid_argument("Index out of range!");
    return BitPointer(x, y, owner);
}

// --------------------------------------------------
// Bitmap
// --------------------------------------------------

Bitmap::Bitmap(int width, int height, bool defaultValue) : width(width), height(height) {
    _dataSize = std::ceil(width * height / 8.0);
    _data = (uint8_t*)std::malloc(_dataSize);
    for (int i = 0; i < _dataSize; i++) {
        if (defaultValue)
            _data[i] = 0xFF;
        else
            _data[i] = 0x00;
    }
}

Bitmap::~Bitmap() {
    if (_data != nullptr)
        std::free(_data);
}

bool Bitmap::get(int x, int y) {
    int index = (y * width + x) / 8;
    int byteIndex = y * width + x - index * 8;
    return (_data[index] >> byteIndex) & 1;
}

void Bitmap::set(int x, int y, bool value) {
    int index = (y * width + x) / 8;
    int byteIndex = y * width + x - index * 8;
    if (value)
        _data[index] |= 1 << byteIndex;
    else
        _data[index] &= ~(1 << byteIndex);
}

BitRow Bitmap::operator[](int y) {
    if (y >= height)
        throw std::invalid_argument("index out of range!");
    return BitRow(y, this);
}

#endif