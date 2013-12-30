#ifndef TIMING_HPP_INCLUDED
#define TIMING_HPP_INCLUDED

#include <sys/time.h>
#include <string>

timeval startTiming();
float getElapsedSec(timeval start);
void outputElapsedSec(std::string desc, timeval start);

#endif // TIMING_HPP_INCLUDED
