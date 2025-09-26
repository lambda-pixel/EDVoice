#ifdef BUILD_MEDICORP

#include "MedicComlpiant.h"


bool MedicCompliant::isCompliant()
{
    return correctIdentifier && !hasWeapons;
}


void MedicCompliant::setShipID(const std::string& shipIdent)
{
    std::string shipIdentLower = shipIdent;
    std::transform(shipIdentLower.begin(), shipIdentLower.end(), shipIdentLower.begin(), ::tolower);

    correctIdentifier = (shipIdentLower == "medic") || (shipIdentLower == "mdic");
}


void MedicCompliant::update(const std::string& event, const std::string& journalEntry) {
    const nlohmann::json json = nlohmann::json::parse(journalEntry);

    if (event == "Loadout") {
        if (json.contains("ShipIdent")) {
            const std::string shipIdent = json["ShipIdent"].get<std::string>();
            setShipID(shipIdent);
        }

        if (json.contains("Modules")) {
            const auto& modules = json["Modules"];
            validateModules(modules);
        }
    }
    else if (event == "SetUserShipName") {
        if (json.contains("UserShipId")) {
            const std::string shipIdent = json["UserShipId"].get<std::string>();
            setShipID(shipIdent);
        }
    }
}


void MedicCompliant::validateModules(const nlohmann::json& modules)
{
    hasWeapons = false;

    for (const auto& module : modules) {
        if (module.contains("Slot")) {
            const std::string slotName = module["Slot"].get<std::string>();

            // Check for TinyHardpoint first, they can be legal
            if (slotName.find("TinyHardpoint") != std::string::npos) {
                if (module.contains("Item")) {
                    const std::string item = module["Item"].get<std::string>();

                    if (item.find("defence_turret") != std::string::npos) {
                        hasWeapons = true;
                    }
                }
            }
            else if (slotName.find("Hardpoint") != std::string::npos) {
                hasWeapons = true;
                return;
            }
        }
    }
}

#endif