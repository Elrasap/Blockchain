#pragma once
#include "alerts/event.hpp"
#include <vector>

class EventCollector {
public:
    void addEvent(const Event& e);
    std::vector<Event> getRecentEvents() const;
private:
    std::vector<Event> events;
};

