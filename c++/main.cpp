#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <deque>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

#define FASTER false // makes it slightly faster but less memory efficient

#if FASTER
#include "include/fastBitmap.h"
#else
#include "include/bitmap.h"
#endif

#define HARDCODE_SIZE true
#define SIZE 6

#define MULTITHREAD false // if it should multithread or not

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
    Candidate(int fieldWidth, int fieldHeight) : map(fieldWidth, fieldHeight), path(fieldWidth * fieldHeight, Pos(-1, -1)) {}
    ~Candidate() {}

    Bitmap map; // contains if a pos has been walked on
    std::vector<Pos> path; // contains the order of of the Poses
    int pathIndex = 0; // the current position in the path vector (instead of push_back)

    void outputInFile(const char* filepath);
};

void solve(int sizeX, int sizeY, std::vector<Pos>* startPoses, std::deque<Candidate>* solutions, std::vector<Pos> deltaDirections);
// if the nextPos creates a valid candidate that is not a solution it adds it to candidates or if its a solution to solutions
void validateAndAdd(std::deque<Candidate>& candidates, std::deque<Candidate>& solutions, std::vector<Pos>& deltaDirections, Candidate candidate, Pos nextPos);
bool checkFinished(Candidate& candidate);
bool connected(Candidate& candidate, std::vector<Pos>& deltaDirections);
int floodFill(Bitmap& toFill, bool valToFill, Pos currPos, std::vector<Pos>& deltaDirections);

std::mutex solutionsMutex; // handels data access to the shared solution vector
std::mutex startPositionsMutex; // handels data access to the shared start positions vector

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

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<Pos> startingPoses;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            startingPoses.push_back(Pos(x, y));

    std::deque<Candidate> solutions;

    // all posible movement directions (in case you also want diagonal too or just diagonal)
    std::vector<Pos> deltaDirections = {Pos(0, -1), Pos(1, 0), Pos(0, 1), Pos(-1, 0)};

#if MULTITHREAD

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 1;
    std::vector<std::thread> threads(numThreads);

    for (int thread = 0; thread < threads.size(); thread++)
        threads[thread] = std::thread(solve, size, size, &startingPoses, &solutions, deltaDirections);

    std::cout << "started " << threads.size() << " threads!" << std::endl;

    for (int thread = 0; thread < threads.size(); thread++) {
        threads[thread].join();
        std::cout << "thread " << thread << " finished!" << std::endl;
    }

#else
    solve(size, size, &startingPoses, &solutions, deltaDirections);
#endif

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    // solutions now contains all possible solutions
    std::cout << "solutions: " << solutions.size() << std::endl;
    std::cout << "time: " << duration.count() << "ms" << std::endl;
}

void Candidate::outputInFile(const char* filepath) {
    std::fstream file(filepath);
    if (!file.good()) {
        std::ofstream(filepath).close();
        file = std::fstream(filepath);
    }
    if (!file) {
        std::cerr << "Error opening file: " << filepath << std::endl;
        return;
    }
    int digits = std::to_string(map.width * map.height - 1).size();
    file << '\n';
    for (int y = 0; y < map.height; y++) {
        for (int x = 0; x < map.width; x++) {
            bool printed = false;
            for (int i = 0; i < path.size() && i < pathIndex; i++) {
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

void solve(int sizeX, int sizeY, std::vector<Pos>* startPoses, std::deque<Candidate>* solutions, std::vector<Pos> deltaDirections) {
    while (true) {
        Pos startPos;
        startPositionsMutex.lock();
        if (startPoses->empty()) {
            startPositionsMutex.unlock();
            break;
        }
        startPos = startPoses->back();
        startPoses->pop_back();
        startPositionsMutex.unlock();

        Candidate initialCandidate(sizeX, sizeY);
        initialCandidate.map[startPos.y][startPos.x] = true;
        initialCandidate.path[initialCandidate.pathIndex] = startPos;
        initialCandidate.pathIndex++;
        std::deque<Candidate> candidates = {initialCandidate};

        while (!candidates.empty()) {
            Candidate currCandidate = candidates.back();
            candidates.pop_back();

            // try to create candidates in each direction
            for (int dir = 0; dir < deltaDirections.size(); dir++) {
                Pos newPos = currCandidate.path[currCandidate.pathIndex - 1] + deltaDirections[dir];
                validateAndAdd(candidates, *solutions, deltaDirections, currCandidate, newPos);
            }
        }
    }
}

void validateAndAdd(std::deque<Candidate>& candidates, std::deque<Candidate>& solutions, std::vector<Pos>& deltaDirections, Candidate candidate, Pos nextPos) {
    if (nextPos.x < 0 || nextPos.x >= candidate.map.width || nextPos.y < 0 || nextPos.y >= candidate.map.height)
        return;
    if (candidate.map[nextPos.y][nextPos.x])
        return;
    candidate.path[candidate.pathIndex] = nextPos;
    candidate.pathIndex++;
    candidate.map[nextPos.y][nextPos.x] = true;
    if (checkFinished(candidate)) {
        std::lock_guard<std::mutex> lock(solutionsMutex);
        solutions.push_back(candidate);
    }
    else if (connected(candidate, deltaDirections))
        candidates.push_back(candidate);
}

bool checkFinished(Candidate& candidate) {
    return candidate.pathIndex >= candidate.map.width * candidate.map.height;
}

bool connected(Candidate& candidate, std::vector<Pos>& deltaDirections) {
    Pos startPos = Pos(0, 0);
    bool found = false;
    for (int y = 0; y < candidate.map.height && !found; y++)
        for (int x = 0; x < candidate.map.width && !found; x++)
            if (!candidate.map[y][x]) {
                startPos = Pos(x, y);
                found = true;
            }

    Bitmap toCheck(candidate.map);
    int numTiles = floodFill(toCheck, false, startPos, deltaDirections);
    return numTiles == (candidate.map.width * candidate.map.height - candidate.pathIndex); // checks if the num of connected tiles is the num of the remaining tiles
}

int floodFill(Bitmap& toFill, bool valToFill, Pos currPos, std::vector<Pos>& deltaDirections) {
    if (currPos.x < 0 || currPos.x >= toFill.width || currPos.y < 0 || currPos.y >= toFill.height)
        return 0;
    if (toFill[currPos.y][currPos.x] != valToFill)
        return 0;
    
    toFill[currPos.y][currPos.x] = !valToFill;
    int sum = 1; // 1 is for this tile
    for (int i = 0; i < deltaDirections.size(); i++)
        sum += floodFill(toFill, valToFill, currPos + deltaDirections[i], deltaDirections);
    return sum;
}