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
public:
    Bitmap(const Bitmap& bitmap); // this constructor ensures that all the data is copied when passed into a function
    Bitmap(int width, int height, bool defaultValue = false);
    ~Bitmap();

    int width, height;

    int dataSize;
    uint8_t* data = nullptr;

    bool get(int x, int y);
    void set(int x, int y, bool value);
#ifdef INCLUDE_STB_IMAGE_WRITE_H // checks if stb_image_write.h was included (this is just to avoid unneccecery headers)
    void outputAsBitmap(const char* filepath);
#endif

    BitRow operator[](int y);
    friend std::ostream& operator<<(std::ostream& os, Bitmap bitmap);
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
    if (x >= owner->width || x < 0)
        throw std::invalid_argument("Index out of range!");
    return BitPointer(x, y, owner);
}

// --------------------------------------------------
// Bitmap
// --------------------------------------------------

Bitmap::Bitmap(const Bitmap& bitmap) {
    width = bitmap.width;
    height = bitmap.height;
    dataSize = bitmap.dataSize;

    data = (uint8_t*)std::malloc(dataSize);
    if (data == nullptr)
        throw std::bad_alloc();
    std::copy(bitmap.data, bitmap.data + dataSize, data);
}

Bitmap::Bitmap(int width, int height, bool defaultValue) : width(width), height(height) {
    dataSize = std::ceil(width * height / 8.0);
    data = (uint8_t*)std::malloc(dataSize);
    for (int i = 0; i < dataSize; i++) {
        if (defaultValue)
            data[i] = 0xFF;
        else
            data[i] = 0x00;
    }
}

Bitmap::~Bitmap() {
    if (data != nullptr)
        std::free(data);
}

bool Bitmap::get(int x, int y) {
    int index = (y * width + x) / 8;
    int byteIndex = y * width + x - index * 8;
    return (data[index] >> byteIndex) & 1;
}

void Bitmap::set(int x, int y, bool value) {
    int index = (y * width + x) / 8;
    int byteIndex = y * width + x - index * 8;
    if (value)
        data[index] |= 1 << byteIndex;
    else
        data[index] &= ~(1 << byteIndex);
}

#ifdef INCLUDE_STB_IMAGE_WRITE_H
void Bitmap::outputAsBitmap(const char* filepath) {
    uint8_t* img = (uint8_t*)std::malloc(width * height);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            img[y * width + x] = get(x, y) * 255;
    stbi_write_bmp(filepath, width, height, 1, img);
    std::free(img);
}
#endif

BitRow Bitmap::operator[](int y) {
    if (y >= height || y < 0)
        throw std::invalid_argument("index out of range!");
    return BitRow(y, this);
}

std::ostream& operator<<(std::ostream& os, Bitmap bitmap) {
    for (int y = 0; y < bitmap.height; y++) {
        for (int x = 0; x < bitmap.width; x++)
            os << (bitmap.get(x, y) ? '#' : '-');
        os << std::endl;
    }
    return os;
}

#endif