#pragma once
#include <string>

class ReleasePublisher {
public:
    static bool publish(const std::string& manifestPath,
                        const std::string& repo,
                        const std::string& tag,
                        const std::string& token);
};

