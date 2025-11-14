#include "security/audit_report.hpp"
#include <fstream>

bool AuditReport::writeJson(const std::string& path, const std::vector<PolicyRule>& rules) {
    std::ofstream out(path);
    if(!out) return false;
    out << "{";
    out << "\"rules\":[";
    for(size_t i=0;i<rules.size();++i){
        const auto& r=rules[i];
        out << "{";
        out << "\"id\":\"" << r.id << "\",";
        out << "\"description\":\"" << r.description << "\",";
        out << "\"passed\":" << (r.passed?"true":"false");
        out << "}";
        if(i+1<rules.size()) out << ",";
    }
    out << "]}";
    return true;
}

