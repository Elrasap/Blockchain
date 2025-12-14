#pragma once
#include <string>

class Logger {
public:
    void info(const std::string& component, const std::string& message);
    void warn(const std::string& component, const std::string& message);
    void error(const std::string& component, const std::string& message);
    void withContext(const std::string& context, const std::string& component);
};

