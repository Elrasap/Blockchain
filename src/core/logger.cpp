#include "core/logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::log(LogLevel lvl, const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");

    std::string levelStr;
    switch (lvl) {
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO:  levelStr = "INFO";  break;
        case LogLevel::WARN:  levelStr = "WARN";  break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
    }

    std::cout << "[" << levelStr << "] " << oss.str() << " " << msg << std::endl;
}

