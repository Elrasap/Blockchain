#include "utils/log.hpp"

std::mutex Logger::mtx;

std::string Logger::levelToStr(LogLevel lvl)
{
    switch (lvl) {
    case LogLevel::INFO:  return "INFO";
    case LogLevel::ERROR: return "ERROR";
    case LogLevel::DEBUG: return "DEBUG";
    }
    return "UNKNOWN";
}

void Logger::log(LogLevel lvl,
                 const std::string& module,
                 const std::string& msg)
{
    std::lock_guard<std::mutex> lock(mtx);

    auto now   = std::chrono::system_clock::now();
    auto t     = std::chrono::system_clock::to_time_t(now);
    auto local = *std::localtime(&t);

    std::cout
        << "[" << std::put_time(&local, "%F %T") << "] "
        << "[" << levelToStr(lvl) << "] "
        << "[" << module << "] "
        << msg
        << "\n";
}

