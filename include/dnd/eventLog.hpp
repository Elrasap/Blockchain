#pragma once
#include <string>
#include <vector>
#include <memory>

class EventLog {
public:
    void recordEvent(const std::string& eventType, const std::string& data);
    std::vector<std::string> queryEvents(const std::string& filter);
    void exportLog(const std::string& path);

};
