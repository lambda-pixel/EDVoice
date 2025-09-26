#pragma once

#include <string>
#include <json.hpp>

#ifdef BUILD_MEDICORP

class MedicCompliant
{
public:
    bool isCompliant();

    void setShipID(const std::string& shipIdent);

    void update(const std::string& event, const std::string& journalEntry);

    void validateModules(const nlohmann::json& modules);

private:
    bool correctIdentifier;
    bool hasWeapons = false;
    std::map<std::string, std::string> equipedModules;
};

#endif