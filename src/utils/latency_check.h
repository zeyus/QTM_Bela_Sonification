// this is how many times to check packet latency
// if it's not defined, there will be no latency check
#define CHECK_CMD_LATENCY 1000

#include <chrono>
#include <array>
#include <iterator>
#include <string>
#include <algorithm>
#include <fstream>

#include "../qsdk/RTProtocol.h"

// hardcoded but we know it exists on bela
std::string gLatency_log_file = "/var/log/qtm_latency.log";
