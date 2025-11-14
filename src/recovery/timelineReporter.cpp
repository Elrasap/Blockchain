#include "recovery/timelineReporter.hpp"
#include <fstream>
#include <iomanip>

bool TimelineReporter::writeJson(const std::string& path, const RecoveryOutcome& outcome) {
    std::ofstream out(path);
    if (!out) return false;
    out << "{\n  \"passed\": " << (outcome.passed ? "true" : "false") << ",\n";
    out << "  \"reason\": \"" << outcome.reason << "\",\n";
    out << "  \"timeline\": [\n";
    for (size_t i = 0; i < outcome.timeline.size(); ++i) {
        const auto& step = outcome.timeline[i];
        auto start = std::chrono::system_clock::to_time_t(step.start);
        auto end = std::chrono::system_clock::to_time_t(step.end);
        out << "    {\n";
        out << "      \"name\": \"" << step.name << "\",\n";
        out << "      \"action\": \"" << step.action << "\",\n";
        out << "      \"start\": \"" << std::ctime(&start);
        out.seekp(-1, std::ios_base::cur); // remove newline from ctime
        out << "\",\n";
        out << "      \"end\": \"" << std::ctime(&end);
        out.seekp(-1, std::ios_base::cur);
        out << "\",\n";
        out << "      \"success\": " << (step.success ? "true" : "false") << ",\n";
        out << "      \"note\": \"" << step.note << "\"\n";
        out << "    }";
        if (i + 1 < outcome.timeline.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    return true;
}

