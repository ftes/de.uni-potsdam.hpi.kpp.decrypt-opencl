#include "timing.hpp"
#include <iostream>

timeval startTiming() {
    timeval start;
    gettimeofday(&start, 0);
    return start;
}

float getElapsedSec(timeval start) {
    timeval end;
    gettimeofday(&end, 0);
    long sec = end.tv_sec - start.tv_sec;
    long usec = end.tv_usec - start.tv_usec;
    return sec + usec / 1000000.f;
}

void outputElapsedSec(std::string desc, timeval start) {
    std::cout << desc << ": " << getElapsedSec(start) << "\n";
}
