#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>

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
    Pos operator+(Pos other) {
        return Pos(x + other.x, y + other.y);
    }
    Pos operator-(Pos other) {
        return Pos(x - other.x, y - other.y);
    }
};

class Candidate {
public:
    Candidate(int fieldWidth, int fieldHeight) : map(fieldWidth, fieldHeight), colCounter(fieldWidth, 0), rowCounter(fieldHeight, 0) {}
    ~Candidate() {}

    Bitmap map; // contains if a pos has been walked on
    std::vector<Pos> path; // contains the order of of the Poses

    std::vector<int> colCounter; // contains an int for each col, that contains the amount of filled spaces in that col
    std::vector<int> rowCounter; // contains an int for each row, that contains the amount of filled spaces in that row

    void outputInFile(const char* filepath);
};

// if the nextPos creates a valid candidate that is not a solution it adds it to candidates or if its a solution to solutions
void validateAndAdd(std::vector<Candidate>& candidates, std::vector<Candidate>& solutions, Candidate candidate, Pos nextPos);
bool checkFilled(Bitmap& map);
bool checkFinished(Candidate& candidate);
bool isSeperated(Candidate& candidate); // checks if the plane has been traversed so that you can't get to all spaces

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

    std::vector<Candidate> candidates;
    std::vector<Candidate> solutions;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            Candidate initailCandiate(size, size);
            initailCandiate.path.push_back(Pos(x, y));
            initailCandiate.map[y][x] = true;
            initailCandiate.rowCounter[y]++;
            initailCandiate.colCounter[x]++;
            candidates.push_back(initailCandiate);
        }
    }

    // all posible movement directions (in case you also want diagonal too or just diagonal)
    std::vector<Pos> deltaDirections = {Pos(0, -1), Pos(1, 0), Pos(0, 1), Pos(-1, 0)};

    while (!candidates.empty()) {
        Candidate currCandidate = candidates.back();
        candidates.pop_back();

        // try to create candidates in each direction
        for (int dir = 0; dir < deltaDirections.size(); dir++) {
            Pos newPos = currCandidate.path.back() + deltaDirections[dir];
            validateAndAdd(candidates, solutions, currCandidate, newPos);
        }
    }

    // solutions now contains all possible solutions
    std::cout << solutions.size() << std::endl;
}

void Candidate::outputInFile(const char* filepath) {
    std::fstream file(filepath);
    if (!file.good()) {
        std::ofstream(filepath).close();
        file = std::fstream(filepath);
    }
    int digits = std::to_string(map.width * map.height - 1).size();
    file << '\n';
    for (int y = 0; y < map.height; y++) {
        for (int x = 0; x < map.width; x++) {
            bool printed = false;
            for (int i = 0; i < path.size(); i++) {
                if (path[i].x == x && path[i].y == y) {
                    file << std::string(digits - std::to_string(i).size(), '0') << i << ' ';
                    printed = true;
                    break;
                }
            }
            if (!printed)
                file << std::string(digits + 1, ' ');
        }
        file << '\n';
    }
    file.close();
}

void validateAndAdd(std::vector<Candidate>& candidates, std::vector<Candidate>& solutions, Candidate candidate, Pos nextPos) {
    if (nextPos.x < 0 || nextPos.x >= candidate.map.width || nextPos.y < 0 || nextPos.y >= candidate.map.height)
        return;
    if (candidate.map[nextPos.y][nextPos.x])
        return;
    candidate.path.push_back(nextPos);
    candidate.map[nextPos.y][nextPos.x] = true;
    candidate.colCounter[nextPos.x]++;
    candidate.rowCounter[nextPos.y]++;
    if (isSeperated(candidate)) // doesn't seem to get to the right number of solutions
        return;
    if (checkFinished(candidate))
        solutions.push_back(candidate);
    else
        candidates.push_back(candidate);
}

bool checkFilled(Bitmap& map) {
    int bitsLeft = map.dataSize * 8 - map.width * map.height;
    uint8_t lastByteMask = 0xFF >> (8 - bitsLeft);
    for (int i = 0; i < map.dataSize - 1; i++)
        if (map.data[i] != 0xFF)
            return false;
    return map.data[map.dataSize - 1] & lastByteMask == lastByteMask;
}

bool checkFinished(Candidate& candidate) {
    return candidate.path.size() >= candidate.map.width * candidate.map.height;
}

bool isSeperated(Candidate& candidate) {
    bool hadEmptyCol = false;
    bool hadFilledCol = false;
    for (int x = 0; x < candidate.colCounter.size(); x++) {
        bool isFilled = candidate.colCounter[x] >= candidate.map.height;
        if (hadEmptyCol && hadFilledCol && !isFilled)
            return true;
        hadEmptyCol = !isFilled || hadEmptyCol;
        hadFilledCol = isFilled || hadFilledCol;
    }
    bool hadEmptyRow = false;
    bool hadFilledRow = false;
    for (int y = 0; y < candidate.rowCounter.size(); y++) {
        bool isFilled = candidate.rowCounter[y] >= candidate.map.width;
        if (hadEmptyRow && hadFilledRow && !isFilled)
            return true;
        hadEmptyRow = !isFilled || hadEmptyRow;
        hadFilledRow = isFilled || hadFilledRow;
    }
    return false;
}