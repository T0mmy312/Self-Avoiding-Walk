#pragma once

#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <iostream>
#include <math.h>
#include <stdint.h>

// this is a faster but less data efficient Bitmap class then bitmap.h
// its made to be as interchagebale possible so you can switch the verion simply with precompiler commands

// ----------------------------------------------------------------------------------------------------
// Bitmap class
// ----------------------------------------------------------------------------------------------------

class Bitmap {
public:
    Bitmap(const Bitmap& bitmap); // this constructor ensures that all the data is copied when passed into a function
    Bitmap(int width, int height, bool defaultValue = false);
    ~Bitmap();

    int width, height;
    bool** data = nullptr;

    bool get(int x, int y);
    void set(int x, int y, bool value);
#ifdef INCLUDE_STB_IMAGE_WRITE_H // checks if stb_image_write.h was included (this is just to avoid unneccecery headers)
    void outputAsBitmap(const char* filepath);
#endif

    bool* operator[](int y);
    friend std::ostream& operator<<(std::ostream& os, Bitmap bitmap);
};

Bitmap::Bitmap(const Bitmap& bitmap) : width(bitmap.width), height(bitmap.height) {
    data = (bool**)std::malloc(height * sizeof(bool*));
    for (int y = 0; y < height; y++) {
        bool* row = (bool*)std::malloc(width * sizeof(bool));
        std::copy(bitmap.data[y], bitmap.data[y] + width, row);
        data[y] = row;
    }
}

Bitmap::Bitmap(int width, int height, bool defaultValue) : width(width), height(height) {
    data = (bool**)std::malloc(height * sizeof(bool*));
    if (height > 0) {
        bool* firstRow = (bool*)std::malloc(width * sizeof(bool));
        for (int x = 0; x < width; x++)
            firstRow[x] = defaultValue;
        data[0] = firstRow;
    }
    for (int y = 1; y < height; y++) {
        bool* row = (bool*)std::malloc(width * sizeof(bool));
        std::copy(data[0], data[0] + width, row);
        data[y] = row;
    }
}

Bitmap::~Bitmap() {
    if (data != nullptr) {
        for (int y = 0; y < height; y++)
            std::free(data[y]);
        std::free(data);
    }
}

bool Bitmap::get(int x, int y) {
    return data[y][x];
}

void Bitmap::set(int x, int y, bool value) {
    data[y][x] = value;
}

#ifdef INCLUDE_STB_IMAGE_WRITE_H
    void Bitmap::outputAsBitmap(const char* filepath) {
        uint8_t* img = (uint8_t*)std::malloc(width * height);
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                img[y * width + x] = data[y][x] * 255;
        stbi_write_bmp(filepath, width, height, 1, img);
        std::free(img);
    }
#endif

bool* Bitmap::operator[](int y) {
    if (y >= height || y < 0)
        throw std::invalid_argument("index out of range!");
    return data[y];
}

std::ostream& operator<<(std::ostream& os, Bitmap bitmap) {
    for (int y = 0; y < bitmap.height; y++) {
        for (int x = 0; x < bitmap.width; x++)
            os << (bitmap.data[y][x] ? '#' : '-');
        os << std::endl;
    }
    return os;
}

#endif