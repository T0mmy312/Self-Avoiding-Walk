#include <iostream>
#include <vector>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"
#include "include/bitmap.h"

#define HARDCODE_SIZE true
#define SIZE 5

class Pos {
public:
    Pos(int x = 0, int y = 0) : x(x), y(y) {}
    ~Pos() {}
    int x, y;
};

class Candidate {
public:
    Candidate(int fieldWidth, int fieldHeight) : map(fieldWidth, fieldHeight) {}
    ~Candidate() {}

    Bitmap map;
    std::vector<Pos> path;
};

int main(int argc, char** argv) {
#if HARDCODE_SIZE
    int size = SIZE;
#else
    int size;
    try { size = std::stoi(argv[0]); }
    catch (...) {
        std::cerr << "Please enter size as an int as the first argument of this programm!" << std::endl;
        return 1;
    }
#endif


}