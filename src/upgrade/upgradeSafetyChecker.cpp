#include "upgrade/upgradeSafetyChecker.hpp"
#include "core/crypto.hpp"
#include <sstream>


SafetyReport UpgradeSafetyChecker::checkUpgrade(const std::string& fromVer,
                                               const std::string& toVer,
                                               GoldenFileManager& gm,
                                               const std::array<uint8_t,32>& currentRoot,
                                               const std::string& currentSchema)
{
    auto ref = gm.readReference(fromVer);
    std::string refRoot = ref["state_root"];
    std::string refSchema = ref["schema"];

    bool schema_ok = (refSchema == currentSchema);
    bool state_ok = (refRoot == crypto::toHex(currentRoot));

    SafetyReport r;
    r.from_version = fromVer;
    r.to_version = toVer;
    r.schema_ok = schema_ok;
    r.state_ok = state_ok;
    r.overall_ok = schema_ok && state_ok;
    return r;
}

