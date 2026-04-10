#include "logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

static std::mutex logMutex;

static std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

static void log(const std::string& level, const std::string& msg, const std::string& ip) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << "[" << timestamp() << "] [" << level << "]";
    if (!ip.empty()) std::cout << " [" << ip << "]";
    std::cout << " " << msg << std::endl;
}

namespace Logger {
    void info(const std::string& msg, const std::string& ip)  { log("INFO",  msg, ip); }
    void warn(const std::string& msg, const std::string& ip)  { log("WARN",  msg, ip); }
    void error(const std::string& msg, const std::string& ip) { log("ERROR", msg, ip); }
}
