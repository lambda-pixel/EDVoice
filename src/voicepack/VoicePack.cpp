#include "VoicePack.h"

#include <fstream>
#include <sstream>
#include <json.hpp>

#include "VoicePackManager.h"
#include "VoicePackUtil.h"

VoicePack::VoicePack(VoicePackManager& voicepackManager)
    : _voicePackManager(voicepackManager)
    , _currVehicle(Vehicle::Ship)
    , _maxShipCargo(0)
    , _currShipCargo(0)
    , _maxSRVCargo(0)
    , _currSRVCargo(0)
    , _medicAcceptedPods({ "occupiedcryopod", "damagedescapepod" })
    , _previousUnderAttack(false)
    , _previousLaunchDrone(false)
    , _previousEjectCargo(false)
    , _isShutdownState(false)
    , _isPriming(false)
{
}


void VoicePack::loadConfig(const std::filesystem::path& filepath)
{
    _configPath = filepath;

    // Clear current configuration
    for (auto& vs : _voiceStatus) {
        vs.fill(VoiceLine());
    }
    
    _voiceSpecial.fill(VoiceLine());
    _voiceJournal.clear();

    for (auto& va : _voiceStatusActive) {
        va.fill(Undefined);
    }

    _voiceSpecialActive.fill(Undefined);
    _voiceJournalActive.clear();

    // Load new configuration
    if (!std::filesystem::exists(filepath)) {
        throw std::runtime_error("VoicePack: Cannot find configuration file: " + filepath.string());
    }

    std::cout << "[INFO  ] Loading voicepack: " << filepath << std::endl;

    std::filesystem::path basePath;

    if (filepath.is_absolute()) {
        basePath = filepath.parent_path();
    }
    else {
        basePath = std::filesystem::current_path() / filepath.parent_path();
    }

    try {
        std::ifstream file(filepath);
        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        nlohmann::json json = nlohmann::json::parse(fileContent);

        // Parse status
        if (json.contains("status")) {
            // Load common status config
            for (size_t v = 0; v < N_Vehicles; v++) {
                loadStatusConfig(basePath, json["status"], _voiceStatus[v]);
            }

            for (size_t v = 0; v < N_Vehicles; v++) {
                const std::string vehicleName = vehicleToString((Vehicle)v);
                if (json["status"].contains(vehicleName)) {
                    loadStatusConfig(basePath, json["status"][vehicleName], _voiceStatus[v]);
                }
            }
        }

        // Parse journal
        if (json.contains("event")) {
            for (auto& je : json["event"].items()) {
                _voiceJournal[je.key()] = VoiceLine(basePath, je.value());
            }
        }

        // Parse journal
        if (json.contains("special")) {
            for (auto& je : json["special"].items()) {
                const std::optional<SpecialEvent> se = specialEventFromString(je.key());

                if (se.has_value()) {
                    if (*se != SpecialEvent::N_SpecialEvents) {
                        _voiceSpecial[*se].loadFromJson(basePath, je.value());
                    }
                }
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

            for (size_t v = 0; v < N_Vehicles; v++) {
                if (!_voiceStatus[v][index].empty()) {
                    const bool hasMissingFiles = _voiceStatus[v][index].removeMissingFiles();

                    if (hasMissingFiles) {
                        std::cout << "[ERR   ] Missing file for status '" << eventName << "' (" << state << ") vehicle '" << vehicleToString((Vehicle)v) << std::endl;
                    }

                    if (_voiceStatus[v][index].empty()) {
                        _voiceStatusActive[v][index] = MissingFile;
                    }                    
                    else {
                        _voiceStatusActive[v][index] = Active;
                    }
                }
                else {
                    _voiceStatusActive[v][index] = Undefined;
                }
            }
        }
    }

    for (auto& [eventName, voicelines] : _voiceJournal) {
        if (!voicelines.empty()) {
            const bool hasMissingFiles = voicelines.removeMissingFiles();

            if (hasMissingFiles) {
                std::cerr << "[ERR   ] Missing file for event '" << eventName << std::endl;
            }

            if (voicelines.empty()) {
                _voiceJournalActive[eventName] = MissingFile;
            }
            else {
                _voiceJournalActive[eventName] = Active;
            }
        }
        else {
            // Should not happen
            assert(0);
            _voiceJournalActive[eventName] = Undefined;
        }
    }

    for (size_t iSpecial = 0; iSpecial < SpecialEvent::N_SpecialEvents; iSpecial++) {
        const std::string eventName = specialEventToString((SpecialEvent)iSpecial);
        auto& path = _voiceSpecial[iSpecial];

        if (!path.empty()) {
            const bool hasMissingFiles = path.removeMissingFiles();

            if (hasMissingFiles) {
                std::cerr << "[ERR   ] Missing file for special '" << eventName << std::endl;
            }

            if (path.empty()) {
                _voiceSpecialActive[iSpecial] = MissingFile;
            }
            else {
                _voiceSpecialActive[iSpecial] = Active;
            }
        }
        else {
            _voiceSpecialActive[iSpecial] = Undefined;
        }
    }
}


void VoicePack::onStatusChanged(StatusEvent event, bool status)
{
    if (_isShutdownState || _isPriming) {
        return;
    }

    if (event >= StatusEvent::N_StatusEvents) {
        return;
    }

    // Prevents voiceline triggered while launching drone
    if (event == StatusEvent::Cargo_Scoop_Deployed && (_previousLaunchDrone || _previousEjectCargo)) {
        return;
    }

    const size_t index = 2 * event + (status ? 1 : 0);

    if (_voiceStatusActive[_currVehicle][index] == Active &&
        _voiceStatus[_currVehicle][index].hasCooledDown()) {
        _voicePackManager.playStatusVoiceline(_currVehicle, event, status, _voiceStatus[_currVehicle][index].getNextVoiceline());
    }
}


void VoicePack::setJournalPreviousEvent(const std::string& event, const std::string& journalEntry)
{
    // Ensure we're updating player status silently (no voiceline triggered)
    onJournalEvent(event, journalEntry);

    // Just for debugging
    std::cout << "[INFO  ] Priming done. Current vehicle: " << vehicleToString(_currVehicle)
              << ", Ship cargo: " << _currShipCargo << "/" << _maxShipCargo
              << ", SRV cargo: " << _currSRVCargo << "/" << _maxSRVCargo
        << std::endl;
}





void VoicePack::onJournalEvent(const std::string& event, const std::string& journalEntry)
{
    if (event == "Shutdown") {
        _isShutdownState = true;
        std::cout << "[INFO  ] Entering shutdown state" << std::endl;
    }
    else if (event == "LoadGame") {
        _isShutdownState = false;
        std::cout << "[INFO  ] Exiting shutdown state" << std::endl;
    }

    // Prevent multiple "under attack" announcements
    if (_previousUnderAttack && event == "UnderAttack") {
        return;
    }

    _previousUnderAttack = (event == "UnderAttack");
    _previousLaunchDrone = (event == "LaunchDrone");

    // Prevent mutilple "cargo scoop deployed" announcements
    // Not reliable for EjectCargo though...
    if (event == "LaunchDrone") {
        _previousLaunchDrone = true;
        std::cout << "[INFO  ] LaunchDrone event received, setting launch state." << std::endl;
    }

    if (event == "EjectCargo") {
        _previousEjectCargo = true;
        std::cout << "[INFO  ] EjectCargo event received, setting eject state." << std::endl;
    }

    // Reset voiceline reading when receiving the acknowledgment
    // from "Cargo" event
    if (_previousEjectCargo || _previousLaunchDrone) {
        if (event == "Cargo") {
            _previousEjectCargo = false;
            _previousLaunchDrone = false;
            std::cout << "[INFO  ] Cargo update received, resetting launch/eject state." << std::endl;
        }
    }

    auto it = _voiceJournal.find(event);

    if (it != _voiceJournal.end() && _voiceJournalActive[event] == Active &&
        it->second.hasCooledDown()) {
        _voicePackManager.playJournalVoiceline(event, it->second.getNextVoiceline());
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
            if (_maxShipCargo > 0 && _currShipCargo == _maxShipCargo) {
                onSpecialEvent(CargoFull);
            }
        }
        // Check for fuel capacity
        if (json.contains("FuelCapacity") && json["FuelCapacity"].contains("Main")) {
            _maxShipFuel = json["FuelCapacity"]["Main"].get<uint32_t>();
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
            if (VoicePackUtil::compareStrings(cargoType, _medicAcceptedPods)
                // TODO Just in case... I don't know all the types like Thargoids pods
                || cargoType.find("pod") != std::string::npos
                || cargoType.find("Pod") != std::string::npos) {
                onSpecialEvent(CollectPod);
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
        
        if (json.contains("SRVType") && srvTypeFromString(json["SRVType"])) {
            const SRVType srvType = srvTypeFromString(json["SRVType"]).value();
            _maxSRVCargo = SRV_MAX_CARGO[srvType];
        }
    }
    else if (event == "DockSRV") {
        setCurrentVehicle(Vehicle::Ship);
    }
    else if (event == "FuelScoop") {
        if (json.contains("Total")) {
            const uint32_t currentFuel = json["Total"].get<uint32_t>();
            if (currentFuel == _maxShipFuel) {
                onSpecialEvent(FuelScoopFinished);
            }
            // This is removed, not reliable right now
            //else if (_canTriggerFuelScooping) {
            //    // Only trigger once per fuel scooping session
            //    onSpecialEvent(FuelScooping);
            //    _canTriggerFuelScooping = false;
            //}
        }
    }
    else if (event == "ReservoirReplenished") {
        if (json.contains("FuelMain")) {
            const uint32_t currentFuel = json["FuelMain"].get<uint32_t>();
            // TODO: special event at X% fuel?
        }
    }
    else if (event == "HullDamage") {
        if (json.contains("Health")) {
            const float health = json["Health"].get<float>();
            if (health < 25E-2f) {
                onSpecialEvent(HullIntegrity_Critical);
            }
            else {
                onSpecialEvent(HullIntegrity_Compromised);
            }
        }
    }
    else if (event == "Liftoff") {
        if (json.contains("PlayerControlled") && !json["PlayerControlled"].get<bool>()) {
            onSpecialEvent(AutoPilot_Liftoff);
        }
    }
    else if (event == "Touchdown") {
        if (json.contains("PlayerControlled") && !json["PlayerControlled"].get<bool>()) {
            onSpecialEvent(AutoPilot_Touchdown);
        }
    }
    //else if (event == "LaunchFighter") {
    //    _currentVehicule = Vehicle::Ship;
    //}
    //else if (event == "DockFighter") {
    //    _currentVehicule = Vehicle::Ship;
    //}
}


void VoicePack::onSpecialEvent(SpecialEvent event)
{
    if (_isPriming || _isShutdownState) {
        return;
    }

    if (_voiceSpecialActive[event] == Active &&
        _voiceSpecial[event].hasCooledDown()) {
        _voicePackManager.playSpecialVoiceline(event, _voiceSpecial[event].getNextVoiceline());
    }
}


void VoicePack::setVoiceStatusState(Vehicle vehicle, StatusEvent event, bool statusState, bool active)
{
    auto& it = _voiceStatusActive[vehicle][2 * event + (statusState ? 1 : 0)];

    if (it != Undefined && it != MissingFile) {
        it = active ? Active : Inactive;
    }
}


void VoicePack::setVoiceJournalState(const std::string& event, bool active)
{
    auto it = _voiceJournalActive.find(event);
 
    if (it != _voiceJournalActive.end() && 
        it->second != Undefined && 
        it->second != MissingFile) {
        it->second = active ? Active : Inactive;
    }
}


void VoicePack::setVoiceSpecialState(SpecialEvent event, bool active)
{
    auto& it = _voiceSpecialActive[event];
    if (it != Undefined && it != MissingFile) {
        it = active ? Active : Inactive;
    }
}


void VoicePack::setShipCargo(uint32_t cargo)
{
    if (cargo != _currShipCargo) {
        // Only trigger if we have a max cargo defined
        if (_maxShipCargo > 0) {
            if (cargo == 0) {
                onSpecialEvent(CargoEmpty);
            }
            else if (cargo == _maxShipCargo) {
                onSpecialEvent(CargoFull);
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
                onSpecialEvent(CargoEmpty);
            }
            else if (cargo == _maxSRVCargo) {
                onSpecialEvent(CargoFull);
            }
        }

        _currSRVCargo = cargo;
    }
}


void VoicePack::setCurrentVehicle(Vehicle vehicle)
{
    if (vehicle != _currVehicle) {
        if (_currVehicle == Vehicle::SRV && vehicle == Vehicle::Ship) {
            // Reset SRV cargo when switching back to ship
            setShipCargo(_currShipCargo + _currSRVCargo);
            setSRVCargo(0);
        }

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
    std::array<VoiceLine, 2 * StatusEvent::N_StatusEvents>& voiceStatus)
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
                voiceStatus[2 * status.value() + 1].loadFromJson(basePath, statusEntry.value());
            }
            else if (statusEntry.key() == "false") {
                voiceStatus[2 * status.value() + 0].loadFromJson(basePath, statusEntry.value());
            }
            else {
                std::cout << "[WARN  ] Unknown status key: " << statusEntry.key() << "\n";
            }
        }
    }
}
