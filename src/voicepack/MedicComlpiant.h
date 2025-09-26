#pragma once

#ifdef BUILD_MEDICORP

#include <string>
#include <json.hpp>

class MedicCompliant
{
public:
    bool isCompliant() const;

    void setShipID(const std::string& shipIdent);

    void update(const std::string& event, const std::string& journalEntry);

    void validateModules(const nlohmann::json& modules);

private:
    bool correctIdentifier;
    bool hasWeapons = false;
    std::map<std::string, std::string> equipedModules;
};

#endif