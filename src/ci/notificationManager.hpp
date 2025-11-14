#include "ci/notification_manager.hpp"
#include <iostream>
#include <cstdlib>

void NotificationManager::sendSuccess(const std::string& msg) {
    std::cout << "[SUCCESS] " << msg << std::endl;
}

void NotificationManager::sendFailure(const std::string& msg) {
    std::cout << "[FAILURE] " << msg << std::endl;
    std::exit(1);
}

