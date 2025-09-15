#include "VoicePack.h"

#include <fstream>
#include <sstream>
#include <json.hpp>


VoicePack::VoicePack()
    : _currVehicle(Vehicle::Ship)
    , _maxShipCargo(0)
    , _currShipCargo(0)
    , _maxSRVCargo(0)
    , _currSRVCargo(0)
    , _previousUnderAttack(false)
{
}


void VoicePack::loadConfig(const char* filepath)
{
    // Clear current configuration
    _voiceStatusCommon.fill(std::filesystem::path());

    for (auto& vs : _voiceStatusSpecial) {
        vs.fill(std::filesystem::path());
    }

    _voiceJournal.clear();

    std::filesystem::path path(filepath);

    // Load new configuration
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("VoicePack: Cannot find configuration file: " + path.string());
    }

    std::cout << "[INFO  ] Loading voicepack: " << filepath << std::endl;

    std::filesystem::path basePath;

    if (path.is_absolute()) {
        basePath = path.parent_path();
    }
    else {
        basePath = std::filesystem::current_path() / path.parent_path();
    }

    try {
        std::ifstream file(filepath);
        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        nlohmann::json json = nlohmann::json::parse(fileContent);

        // Parse status
        if (json.contains("status")) {
            // Load common status config
            loadStatusConfig(basePath, json["status"], _voiceStatusCommon);

            for (size_t v = 0; v < N_Vehicles; v++) {
                const std::string vehicleName = vehicleToString((Vehicle)v);
                if (json["status"].contains(vehicleName)) {
                    loadStatusConfig(basePath, json["status"][vehicleName], _voiceStatusSpecial[v]);
                }
            }
        }

        // Parse journal
        if (json.contains("event")) {
            for (auto& je : json["event"].items()) {
                _voiceJournal[je.key()] = resolvePath(basePath, je.value().get<std::string>());
            }
        }

        // Parse journal
        if (json.contains("special")) {
            for (auto& je : json["special"].items()) {
                _voiceSpecial[je.key()] = resolvePath(basePath, je.value().get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERR] JSON: " << e.what() << "\n";
    }

    // Log missing files and remove them from the list
    for (size_t iEvent = 0; iEvent < StatusEvent::N_StatusEvents; iEvent++) {
        const std::string eventName = statusToString((StatusEvent)iEvent);

        for (size_t i = 0; i < 2; i++) {
            const std::string state = (i == 0) ? "false" : "true";
            const size_t index = 2 * iEvent + i;

            if (!_voiceStatusCommon[index].empty() && 
                !std::filesystem::exists(_voiceStatusCommon[index])) {
                std::cout << "[ERR   ] Missing file for status '" << eventName << "' (" << state << "): " << _voiceStatusCommon[index] << std::endl;
                _voiceStatusCommon[index].clear();
            }

            for (size_t v = 0; v < N_Vehicles; v++) {
                if (!_voiceStatusSpecial[v][index].empty() &&
                    !std::filesystem::exists(_voiceStatusSpecial[v][index])) {
                    std::cout << "[ERR   ] Missing file for status '" << eventName << "' (" << state << ") vehicle '" << vehicleToString((Vehicle)v) << "': " << _voiceStatusSpecial[v][index] << std::endl;
                    _voiceStatusSpecial[v][index].clear();
                }
            }
        }
    }

    for (auto& [event, path] : _voiceJournal) {
        if (!path.empty() && !std::filesystem::exists(path)) {
            std::cerr << "[ERR   ] Missing file for event '" << event << "': " << path << std::endl;
            path.clear();
        }
    }

    for (auto& [event, path] : _voiceSpecial) {
        if (!path.empty() && !std::filesystem::exists(path)) {
            std::cerr << "[ERR   ] Missing file for special '" << event << "': " << path << std::endl;
            path.clear();
        }
    }
}


void VoicePack::onStatusChanged(StatusEvent event, bool status)
{
    if (event >= StatusEvent::N_StatusEvents) {
        return;
    }

    const size_t index = 2 * event + (status ? 1 : 0);

    if (!_voiceStatusCommon[index].empty()) {
        _player.addTrack(_voiceStatusCommon[index]);
    }

    if (!_voiceStatusSpecial[_currVehicle][index].empty()) {
        _player.addTrack(_voiceStatusSpecial[_currVehicle][index]);
    }
}


void VoicePack::onJournalEvent(const std::string& event, const std::string& journalEntry)
{
    // Prevent multiple "under attack" announcements
    if (_previousUnderAttack && event == "UnderAttack") {
        return;
    }

    _previousUnderAttack = (event == "UnderAttack");

    auto it = _voiceJournal.find(event);

    if (it != _voiceJournal.end()) {
        _player.addTrack(it->second);
    }

    const nlohmann::json json = nlohmann::json::parse(journalEntry);

    // Check for cargo capacity change, there no specific event for max cargo change
    if (event == "Loadout") {
        // Check for cargo capacity
        if (json.contains("CargoCapacity")) {
            const uint32_t cargo = json["CargoCapacity"].get<uint32_t>();
            if (cargo != _maxShipCargo) {
                _maxShipCargo = cargo;
                std::cout << "[INFO  ] New cargo capacity: " << _maxShipCargo << std::endl;
            }

            // Check on ship swap if we've reached the max cargo capacity
            if (_currShipCargo == _maxShipCargo) {
                onSpecialEvent("CargoFull");
            }
        }
    }
    else if (event == "Cargo") {
        // Check for cargo change
        if (json.contains("Count")) {
            if (json.contains("Vessel")) {
                const uint32_t cargo = json["Count"].get<uint32_t>();
                if (json["Vessel"] == "Ship") {
                    setShipCargo(cargo);
                }
                else if (json["Vessel"] == "SRV") {
                    setSRVCargo(cargo);
                }
            }
        }
    }
    else if (event == "CollectCargo") {
        if (json.contains("Type")) {
            const std::string cargoType = json["Type"].get<std::string>();

            // We've collected an escape pod!
            if (cargoType == "occupiedcryopod"
                || cargoType == "damagedescapepod"
                // TODO Just in case... I don't know all the types like Thargoids pods
                || cargoType.find("pod") != std::string::npos) {
                onSpecialEvent("CollectPod");
            }
        }
    }
    // On foot transitions
    else if (event == "Disembark") {
        setCurrentVehicle(Vehicle::OnFoot);
    }
    else if (event == "Embark") {
        if (json["SRV"].get<bool>()) {
            setCurrentVehicle(Vehicle::SRV);
        }
        else {
            setCurrentVehicle(Vehicle::Ship);
        }
    }
    else if (event == "LaunchSRV") {
        setCurrentVehicle(Vehicle::SRV);
    }
    else if (event == "DockSRV") {
        setCurrentVehicle(Vehicle::Ship);
    }
    //else if (event == "LaunchFighter") {
    //    _currentVehicule = Vehicle::Ship;
    //}
    //else if (event == "DockFighter") {
    //    _currentVehicule = Vehicle::Ship;
    //}
}


void VoicePack::onSpecialEvent(const std::string& event)
{
    auto it = _voiceSpecial.find(event);

    if (it != _voiceSpecial.end()) {
        _player.addTrack(it->second);
    }
}


void VoicePack::setShipCargo(uint32_t cargo)
{
    if (cargo != _currShipCargo) {
        // Only trigger if we have a max cargo defined
        if (_maxShipCargo > 0) {
            if (cargo == 0) {
                onSpecialEvent("CargoEmpty");
            }
            else if (cargo == _maxShipCargo) {
                onSpecialEvent("CargoFull");
            }
        }

        _currShipCargo = cargo;
    }
}


void VoicePack::setSRVCargo(uint32_t cargo)
{
    if (cargo != _currSRVCargo) {
        // Only trigger if we have a max cargo defined
        if (_maxSRVCargo > 0) {
            if (cargo == 0 && _maxSRVCargo > 0) {
                onSpecialEvent("CargoEmpty");
            }
            else if (cargo == _maxSRVCargo) {
                onSpecialEvent("CargoFull");
            }
        }

        _currSRVCargo = cargo;
    }
}


void VoicePack::setCurrentVehicle(Vehicle vehicle)
{
    if (vehicle != _currVehicle) {
        _currVehicle = vehicle;

        switch (_currVehicle) {
        case Ship:
            std::cout << "[INFO  ] Now in ship" << std::endl;
            break;
        case SRV:
            std::cout << "[INFO  ] Now in SRV" << std::endl;
            break;
        case OnFoot:
            std::cout << "[INFO  ] Now on foot" << std::endl;
            break;
        }
    }
}


void VoicePack::loadStatusConfig(
    const std::filesystem::path& basePath,
    const nlohmann::json& json,
    std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents>& voiceStatus)
{
    for (auto& st : json.items()) {
        const std::optional<StatusEvent> status = statusFromString(st.key());

        if (!status || status == StatusEvent::N_StatusEvents) {
            // This is a nested status probably
            // std::cout << "[WARN  ] Unknown status event: " << st.key() << "\n";
            continue;
        }

        for (auto& statusEntry : st.value().items()) {
            if (statusEntry.key() == "true") {
                voiceStatus[2 * status.value() + 1] = resolvePath(basePath, statusEntry.value().get<std::string>());
            }
            else if (statusEntry.key() == "false") {
                voiceStatus[2 * status.value() + 0] = resolvePath(basePath, statusEntry.value().get<std::string>());
            }
            else {
                std::cout << "[WARN  ] Unknown status key: " << statusEntry.key() << "\n";
            }
        }
    }
}

std::filesystem::path VoicePack::resolvePath(
    const std::filesystem::path& basePath,
    const std::string& file)
{
    std::filesystem::path p(file);
    return p.is_absolute() ? p : (basePath / p);
}


// ---- Medic Compliant class implementation ----


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


// ---- Alta class implementation ----


Alta::Alta()
    : _altaLoaded(false)
{

}


void Alta::loadConfig(const char* filepath)
{
    // small hack to get the path of voicepack
    std::filesystem::path path(filepath);
    std::filesystem::path basePath;

    if (path.is_absolute()) {
        basePath = path.parent_path();
    }
    else {
        basePath = std::filesystem::current_path() / path.parent_path();
    }

    _standardVoicePack.loadConfig(filepath);

    // TODO: This is ultra hacky:
    // Attempt to locate ALTA voicepack from the provided config file.
    // In case the ALTA voicepack is not next to it, this will shamefully crash
    // Also, prevent from reloading ALTA mehhh prevent crashing in case the user
    // loads another voicepack with drag'n drop.
    // I'll attempt something more elegant later, sorry folks!
    if (!_altaLoaded) {
        const std::filesystem::path altaVoicepack = basePath.parent_path() / "ALTA" / "config.json";

        _altaVoicePack.loadConfig(altaVoicepack.string().c_str());
        _altaActive = false;
        std::cout << "[INFO  ] ALTA voicepack loaded." << std::endl;

        _altaLoaded = true;
    }
}


void Alta::onStatusChanged(StatusEvent event, bool status)
{
    if (_altaActive) {
        _altaVoicePack.onStatusChanged(event, status);
    }
    else {
        _standardVoicePack.onStatusChanged(event, status);
    }
}


void Alta::onJournalEvent(const std::string& event, const std::string& journalEntry)
{
    _medicCompliant.update(event, journalEntry);
    const bool compliant = _medicCompliant.isCompliant();

    // Check change of status
    if (compliant != _altaActive) {
        if (!_altaActive) {
            // We are activating ALTA
            std::cout << "[INFO  ] ALTA voicepack activated." << std::endl;
            _altaVoicePack.transferSettings(_standardVoicePack);
            _altaVoicePack.onSpecialEvent("Activating");
        }
        else {
            // We are deactivating ALTA
            std::cout << "[INFO  ] Standard voicepack activated." << std::endl;
            _standardVoicePack.transferSettings(_altaVoicePack);
            _altaVoicePack.onSpecialEvent("Deactivating");
        }

        _altaActive = compliant;
    }

    if (_altaActive) {
        _altaVoicePack.onJournalEvent(event, journalEntry);
    }
    else {
        _standardVoicePack.onJournalEvent(event, journalEntry);
    }
}