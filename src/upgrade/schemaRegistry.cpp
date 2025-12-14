#include "upgrade/schemaRegistry.hpp"

SchemaRegistry& SchemaRegistry::instance() {
    static SchemaRegistry inst;
    return inst;
}

void SchemaRegistry::registerSchema(int version, const std::string& hash, const std::string& changes) {
    SchemaInfo info;
    info.version = version;
    info.hash = hash;
    info.changes = changes;
    schemas[version] = info;
}

const SchemaInfo& SchemaRegistry::get(int version) const {
    return schemas.at(version);
}

bool SchemaRegistry::compatible(int v1, int v2) const {
    const SchemaInfo& s1 = schemas.at(v1);
    const SchemaInfo& s2 = schemas.at(v2);
    if (s1.hash == s2.hash) return true;
    if (s1.changes.find("non-breaking") != std::string::npos) return true;
    if (s2.changes.find("non-breaking") != std::string::npos) return true;
    return false;
}

