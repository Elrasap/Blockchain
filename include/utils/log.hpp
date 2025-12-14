#pragma once
#include <string>
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    INFO,
    ERROR,
    DEBUG
};

class Logger {
public:
    static void log(LogLevel lvl, const std::string& module, const std::string& msg);

private:
    static std::mutex mtx;
    static std::string levelToStr(LogLevel lvl);
};

#define LOG_INFO(module, msg)  Logger::log(LogLevel::INFO,  module, msg)
#define LOG_ERROR(module, msg) Logger::log(LogLevel::ERROR, module, msg)
#define LOG_DEBUG(module, msg) Logger::log(LogLevel::DEBUG, module, msg)

