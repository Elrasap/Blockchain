#pragma once
#include <string>

class NotificationManager {
public:
    static void sendSuccess(const std::string& msg);
    static void sendFailure(const std::string& msg);
};

