#include "util.h"

string getEpochTime() {
    timeval time;
	gettimeofday(&time, NULL);
    return to_string(time.tv_sec) + "." + to_string(time.tv_usec).substr(0, 2);
}