#pragma once
#include <string>

class Tracing {
public:
    void beginSpan(const std::string& name);
    void endSpan();
    void annotate(const std::string& name, const std::string& value);
};

