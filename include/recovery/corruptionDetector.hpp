#pragma once

class CorruptionDetector{

public:
    bool detectBrokenLinks();
    bool detectChecksumMismatch();
    bool detectRollbackViolation();
    void quarantineNode();
};
