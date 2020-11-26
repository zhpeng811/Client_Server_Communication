/**
 * util.cpp
 * Ze Hui Peng(zhpeng)
 * Fall 2020 CMPUT 379 Assignment 3
 */

#include "util.h"

string getEpochTime() {
    timeval time;
	gettimeofday(&time, NULL);
    return to_string(time.tv_sec) + "." + to_string(time.tv_usec).substr(0, 2);
}