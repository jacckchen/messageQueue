#include <iostream>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "time_stamp.h"
std::string get_iso8601_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d-%H:%M:%S");
    return ss.str();
}