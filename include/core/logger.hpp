#pragma once
#include <string>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    static Logger& instance();
    void log(LogLevel lvl, const std::string& msg);
private:
    Logger() = default;
};

