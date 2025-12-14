#pragma once
#include <string>
#include <map>

struct SchemaInfo {
    int version;
    std::string hash;
    std::string changes;
};

class SchemaRegistry {
public:
    static SchemaRegistry& instance();
    void registerSchema(int version, const std::string& hash, const std::string& changes);
    const SchemaInfo& get(int version) const;
    bool compatible(int v1, int v2) const;
private:
    SchemaRegistry() = default;
    std::map<int, SchemaInfo> schemas;
};

