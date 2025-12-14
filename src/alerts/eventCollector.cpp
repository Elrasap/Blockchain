#include "alerts/eventCollector.hpp"

void EventCollector::addEvent(const Event& e) {
    events.push_back(e);
    if (events.size() > 1000) events.erase(events.begin());
}

std::vector<Event> EventCollector::getRecentEvents() const {
    return events;
}

