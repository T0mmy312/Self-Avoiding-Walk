#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <deque>
#include <functional>
#include <filesystem>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

#define FASTER true // makes it slightly faster but less memory efficient

#if FASTER
#include "include/fastBitmap.h"
#else
#include "include/bitmap.h"
#endif

#define HARDCODE_SIZE false
#define SIZE 5

#define MULTITHREAD true // if it should multithread or not

#define OUTPUT_SOLUTIONS_PER_SQARE true // just the number of solutions where the starting position is the current sqare
#define OUTPUT_SOLUTIONS_IN_FILE true

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

    friend std::ostream& operator<<(std::ostream& os, Candidate& can);
};

void solve(int sizeX, int sizeY, std::vector<Pos>* startPoses, std::deque<Candidate>* solutions, std::vector<Pos> deltaDirections);
// if the nextPos creates a valid candidate that is not a solution it adds it to candidates or if its a solution to solutions
void validateAndAdd(std::deque<Candidate>& candidates, std::deque<Candidate>& solutions, std::vector<Pos>& deltaDirections, Candidate candidate, Pos nextPos);
bool checkFinished(Candidate& candidate);
bool connected(Candidate& candidate, std::vector<Pos>& deltaDirections);
int floodFill(Bitmap& toFill, bool valToFill, Pos currPos, std::vector<Pos>& deltaDirections);
Candidate applyToEntirePath(Candidate candidate, std::function<Pos(Pos, int, Candidate&)> func); // creates a copy of the candidat and applies the function to the path
// the function should take in the current pos in path the index of the pos and the Candidate and return the new pos

std::mutex solutionsMutex; // handels data access to the shared solution vector
std::mutex startPositionsMutex; // handels data access to the shared start positions vector

int main(int argc, char** argv) {
#if HARDCODE_SIZE
    int size = SIZE;
#else
    int size;
    try { size = std::stoi(argv[1]); }
    catch (...) {
        std::cerr << "Please enter size as an int as the first argument of this programm!" << std::endl;
        return 1;
    }
#endif

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<Pos> startingPoses;
    for (int x = 0; x < std::ceil(size / 2.0); x++) {
        for (int y = 0; y <= x; y++) {
            if ((x + y) % 2 == 1 && size % 2 == 1)
                continue;
            startingPoses.push_back(Pos(x, y));
        }
    }

    std::deque<Candidate> solutions;

    // all posible movement directions (in case you also want diagonal too or just diagonal)
    std::vector<Pos> deltaDirections = {Pos(0, -1), Pos(1, 0), Pos(0, 1), Pos(-1, 0)};
    // std::vector<Pos> deltaDirections = {Pos(0, -1), Pos(1, 0), Pos(0, 1), Pos(-1, 0), Pos(1, -1), Pos(1, 1), Pos(-1, 1), Pos(-1, -1)}; // included diagonal Movement

#if MULTITHREAD

    unsigned int numThreads = std::min((size_t)std::thread::hardware_concurrency(), startingPoses.size());
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

    std::deque<Candidate> allSolutions;

    // if x == y you only have to mirror it over x, y and xy
    // if x != y you have to create a new solution by swapping x and y and mirroring it over x, y and xy 
    // if x == size / 2.0 you don't mirror vertically
    // if y == size / 2.0 you don't mirror horizontally
    while (!solutions.empty()) {
        std::vector<Candidate> currSolution = {solutions.back()};
        solutions.pop_back();

        if (currSolution[0].path[0].x != currSolution[0].path[0].y)
            currSolution.push_back(applyToEntirePath(currSolution[0], [](Pos curr, int i, Candidate c) -> Pos {
                return Pos(curr.y, curr.x);
            }));
        
        for (int i = 0; i < currSolution.size(); i++) {
            int cases = 0;
            if (currSolution[i].path[0].x != (currSolution[i].map.width - 1) / 2.0) {
                cases++;
                allSolutions.push_back(applyToEntirePath(currSolution[i], [](Pos curr, int i, Candidate& c) -> Pos {
                    return Pos(c.map.width - curr.x - 1, curr.y);
                }));
            }
            if (currSolution[i].path[0].y != (currSolution[i].map.height - 1) / 2.0) {
                cases++;
                allSolutions.push_back(applyToEntirePath(currSolution[i], [](Pos curr, int i, Candidate& c) -> Pos {
                    return Pos(curr.x, c.map.height - curr.y - 1);
                }));
            }
            if (cases == 2)
                allSolutions.push_back(applyToEntirePath(currSolution[i], [](Pos curr, int i, Candidate& c) -> Pos {
                    return Pos(c.map.width - curr.x - 1, c.map.height - curr.y - 1);
                }));
            allSolutions.push_back(currSolution[i]);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    // allSolutions now contains all possible solutions
    std::cout << "solutions: " << allSolutions.size() << std::endl;
    std::cout << "time: " << duration.count() << "ms" << std::endl;

    auto outputStart = std::chrono::high_resolution_clock::now();

#if OUTPUT_SOLUTIONS_PER_SQARE
    std::filesystem::create_directory("solPerSqr");
    std::ofstream solPerSqrOutput("solPerSqr/solPerSqr" + std::to_string(size) + "x" + std::to_string(size) + ".txt");
    std::vector<std::vector<int>> solutionsPerSqare(size, std::vector<int>(size, 0));
    for (int i = 0; i < allSolutions.size(); i++)
        solutionsPerSqare[allSolutions[i].path[0].y][allSolutions[i].path[0].x]++;

    int maxDigits = 0;
    for (int y = 0; y < solutionsPerSqare.size(); y++)
        for (int x = 0; x < solutionsPerSqare[y].size(); x++)
            maxDigits = std::max(std::to_string(solutionsPerSqare[y][x]).size(), (size_t)maxDigits);
    
    for (int y = 0; y < solutionsPerSqare.size(); y++) {
        for (int x = 0; x < solutionsPerSqare[y].size(); x++) {
            solPerSqrOutput << std::string(maxDigits - std::to_string(solutionsPerSqare[y][x]).size(), '0');
            solPerSqrOutput << solutionsPerSqare[y][x] << ' ';
        }
        solPerSqrOutput << '\n';
    }
    solPerSqrOutput.close();
#endif

#if OUTPUT_SOLUTIONS_IN_FILE
    std::vector<std::string> numberTranslation(size * size, "0");
    int numDigits = std::to_string(size * size - 1).size();
    for (int i = 0; i < numberTranslation.size(); i++)
        numberTranslation[i] = std::string(numDigits - std::to_string(i).size(), '0') + std::to_string(i);
    std::string none(numDigits, '-');

    std::filesystem::create_directory("out");
    std::ofstream file("out/output" + std::to_string(size) + "x" + std::to_string(size) + ".txt");
    for (int i = 0; i < allSolutions.size(); i++) {
        std::vector<std::vector<std::string>> values(allSolutions[i].map.height, std::vector<std::string>(allSolutions[i].map.width, none));
        for (int p = 0; p < allSolutions[i].path.size() && p < allSolutions[i].pathIndex; p++)
            values[allSolutions[i].path[p].y][allSolutions[i].path[p].x] = numberTranslation[p];
        for (int y = 0; y < allSolutions[i].map.height; y++) {
            for (int x = 0; x < allSolutions[i].map.width; x++)
                file << values[y][x] << " ";
            file << "\n";
        }
        file << "\n";
    }
    file.close();
#endif

    auto outputEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> outputDuration = outputEnd - outputStart;

    std::cout << "time to write to file: " << outputDuration.count() << "ms" << std::endl;
}

std::ostream& operator<<(std::ostream& os, Candidate& can) {
    int digits = std::to_string(can.map.width * can.map.height - 1).size();
    os << std::endl;
    std::vector<std::vector<std::string>> values(can.map.height, std::vector<std::string>(can.map.width, std::string(digits, '-')));
    for (int i = 0; i < can.path.size() && i < can.pathIndex; i++) {
        std::string iString = std::to_string(i);
        values[can.path[i].y][can.path[i].x] = std::string(digits - iString.size(), '0') + iString;
    }
    for (int y = 0; y < can.map.height; y++) {
        for (int x = 0; x < can.map.width; x++)
            os << values[y][x] << " ";
        os << std::endl;
    }
    return os;
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
#if FASTER // this is just for memory optimization, becuase the map is no longer needed after its a solution
        if (candidate.map.data != nullptr) {
            for (int y = 0; y < candidate.map.height; y++)
                std::free(candidate.map.data[y]);
            std::free(candidate.map.data);
            candidate.map.data = nullptr;
        }
#else
        if (candidate.map.data != nullptr)
            std::free(candidate.map.data);
        candidate.map.data = nullptr;
#endif
        std::lock_guard<std::mutex> lock(solutionsMutex);
        solutions.push_back(candidate); // maybe make each thread return its solution list so you don't have to use the mutex
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

Candidate applyToEntirePath(Candidate candidate, std::function<Pos(Pos, int, Candidate&)> func) {
    for (int i = 0; i < candidate.path.size() && i < candidate.pathIndex; i++)
        candidate.path[i] = func(candidate.path[i], i, candidate);
    return candidate;
}