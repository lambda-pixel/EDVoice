#include "VoicePackManager.h"

#include <iostream>
#include <fstream>

#include "../util/EliteFileUtil.h"

VoicePackManager::VoicePackManager()
    : _altaLoaded(false)
    , _standardVoicePack(*this)
    , _medicVoicePack(*this)
{
}


void VoicePackManager::loadConfig(const char* filepath)
{
    if (!std::filesystem::exists(filepath)) {
        throw std::runtime_error("Cannot find configuration file: " + std::string(filepath));
    }

    std::filesystem::path path(filepath);
    std::filesystem::path basePath;

    if (path.is_absolute()) {
        basePath = path.parent_path();
    }
    else {
        basePath = std::filesystem::current_path() / path.parent_path();
    }

    // Clear current configuration
    for (auto& va : _configVoiceStatusActive) {
        va.fill(Undefined);
    }

    _configVoiceJournalActive.clear();
    _configVoiceSpecialActive.fill(Undefined);

    // Load installed voicepacks
    std::ifstream file(filepath);
    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    nlohmann::json json = nlohmann::json::parse(fileContent);

    if (json.contains("voicepacks")) {
        for (auto& vp : json["voicepacks"].items()) {
            const std::string& value = vp.value().get<std::string>();
            const std::filesystem::path vpPath = EliteFileUtil::resolvePath(basePath, value);

            if (std::filesystem::exists(vpPath)) {
                // We keep the path the same as in the config file
                _installedVoicePacks[vp.key()] = value;
                std::cout << "[INFO  ] Found installed voicepack: " << vp.key() << " at " << vpPath << std::endl;
            }
            else {
                std::cerr << "[ERR   ] Cannot find installed voicepack: " << vp.key() << " at " << vpPath << std::endl;
            }
        }
    }

    // Load standard voicepack
    if (json.contains("defaultVoicePack")) {
        const std::string& defaultVP = json["defaultVoicePack"].get<std::string>();
        const auto& it = _installedVoicePacks.find(defaultVP);

        if (it != _installedVoicePacks.end()) {
            const std::filesystem::path vpPath = EliteFileUtil::resolvePath(basePath, it->second);

            std::cout << "[INFO  ] Loading default voicepack: " << defaultVP << std::endl;
            _standardVoicePack.loadConfig(vpPath);
        }
        else {
            std::cerr << "[ERR   ] Cannot find default voicepack: " << defaultVP << std::endl;
        }
    }

    // Load medic voicepack
    if (json.contains("medicVoicePack")) {
        const std::string& defaultVP = json["medicVoicePack"].get<std::string>();
        const auto& it = _installedVoicePacks.find(defaultVP);

        if (it != _installedVoicePacks.end()) {
            const std::filesystem::path vpPath = EliteFileUtil::resolvePath(basePath, it->second);

            std::cout << "[INFO  ] Loading default voicepack: " << defaultVP << std::endl;
            _medicVoicePack.loadConfig(vpPath);
        }
        else {
            std::cerr << "[ERR   ] Cannot find default voicepack: " << defaultVP << std::endl;
        }
    }

    // Load activation status
    if (json.contains("activeVoiceActions")) {
        const nlohmann::json jsonActiveVoiceActions = json["activeVoiceActions"];

        if (jsonActiveVoiceActions.contains("status")) {
            const nlohmann::json jsonStatusVoiceActions = jsonActiveVoiceActions["status"];

            for (uint32_t iVehicle = 0; iVehicle < N_Vehicles; iVehicle++) {
                const std::string vehicleName = vehicleToString((Vehicle)iVehicle);

                if (jsonStatusVoiceActions.contains(vehicleName)) {
                    const nlohmann::json jsonVehicleStatusVoiceActions = jsonStatusVoiceActions[vehicleName];

                    for (auto& st : jsonVehicleStatusVoiceActions.items()) {
                        const std::optional<StatusEvent> status = statusFromString(st.key());
                        if (!status || status == StatusEvent::N_StatusEvents) {
                            // This is a nested status probably
                            // std::cout << "[WARN  ] Unknown status event: " << st.key() << "\n";
                            continue;
                        }
                        for (auto& statusEntry : st.value().items()) {
                            size_t iEvent = 0;

                            if (statusEntry.key() == "true") {
                                iEvent = indexFromStatusEvent(status.value(), true);
                            }
                            else if (statusEntry.key() == "false") {
                                iEvent = indexFromStatusEvent(status.value(), false);
                            }
                            else {
                                std::cout << "[WARN  ] Unknown status key: " << statusEntry.key() << "\n";
                                continue;
                            }

                            if (statusEntry.value().get<bool>()) {
                                _configVoiceStatusActive[iVehicle][iEvent] = Active;
                            }
                            else {
                                _configVoiceStatusActive[iVehicle][iEvent] = Inactive;
                            }
                        }
                    }
                }
            }
        }

        if (jsonActiveVoiceActions.contains("event")) {
            const nlohmann::json jsonEventVoiceActions = jsonActiveVoiceActions["event"];

            for (auto& je : jsonEventVoiceActions.items()) {
                if (je.value().get<bool>()) {
                    _configVoiceJournalActive[je.key()] = Active;
                }
                else {
                    _configVoiceJournalActive[je.key()] = Inactive;
                }
            }
        }

        if (jsonActiveVoiceActions.contains("special")) {
            const nlohmann::json jsonSpecialVoiceActions = jsonActiveVoiceActions["special"];
            for (auto& se : jsonSpecialVoiceActions.items()) {
                const std::optional<SpecialEvent> event = specialEventFromString(se.key());

                if (event == SpecialEvent::N_SpecialEvents) {
                    std::cout << "[WARN  ] Unknown special event: " << se.key() << "\n";
                    continue;
                }

                if (se.value().get<bool>()) {
                    _configVoiceSpecialActive[event.value()] = Active;
                }
                else {
                    _configVoiceSpecialActive[event.value()] = Inactive;
                }
            }
        }
    }

    // Merge back potentially new actions from the standard voicepack

    VoicePack& vp = _standardVoicePack;

    {
        // 1 - apply to voicepacks
        std::array<std::array<VoiceTriggerStatus, 2 * StatusEvent::N_StatusEvents>, N_Vehicles>& voiceStatusActive = vp.getVoiceStatusActive();
        std::map<std::string, VoiceTriggerStatus>& voiceJournalActive = vp.getVoiceJournalActive();
        std::array<VoiceTriggerStatus, N_SpecialEvents>& voiceSpecialActive = vp.getVoiceSpecialActive();

        for (uint32_t iVehicle = 0; iVehicle < N_Vehicles; iVehicle++) {
            for (uint32_t iEvent = 0; iEvent < 2 * StatusEvent::N_StatusEvents; iEvent++) {
                if (_configVoiceStatusActive[iVehicle][iEvent] != Undefined && _configVoiceStatusActive[iVehicle][iEvent] != MissingFile) {
                    voiceStatusActive[iVehicle][iEvent] = _configVoiceStatusActive[iVehicle][iEvent];
                }
            }
        }

        for (auto& je : _configVoiceJournalActive) {
            if (je.second != Undefined && je.second != MissingFile) {
                voiceJournalActive[je.first] = je.second;
            }
        }

        for (uint32_t iEvent = 0; iEvent < N_SpecialEvents; iEvent++) {
            if (_configVoiceSpecialActive[iEvent] != Undefined && _configVoiceSpecialActive[iEvent] != MissingFile) {
                voiceSpecialActive[iEvent] = _configVoiceSpecialActive[iEvent];
            }
        }

        // 2 - get unknown new events from voicepack
        for (uint32_t iVehicle = 0; iVehicle < N_Vehicles; iVehicle++) {
            for (uint32_t iEvent = 0; iEvent < 2 * StatusEvent::N_StatusEvents; iEvent++) {
                if (_configVoiceStatusActive[iVehicle][iEvent] == Undefined) {
                    _configVoiceStatusActive[iVehicle][iEvent] = voiceStatusActive[iVehicle][iEvent];
                }
            }
        }

        for (auto& je : voiceJournalActive) {
            if (_configVoiceJournalActive.find(je.first) == _configVoiceJournalActive.end()) {
                _configVoiceJournalActive[je.first] = je.second;
            }
        }

        for (uint32_t iEvent = 0; iEvent < N_SpecialEvents; iEvent++) {
            if (_configVoiceSpecialActive[iEvent] == Undefined) {
                _configVoiceSpecialActive[iEvent] = voiceSpecialActive[iEvent];
            }
        }
    }
    // TODO!!
}


void VoicePackManager::onStatusChanged(StatusEvent event, bool status)
{
    // Ignore status change in shutdown state
    if (_isShutdownState) {
        return;
    }

    if (_altaActive) {
        _medicVoicePack.onStatusChanged(event, status);
    }
    else {
        _standardVoicePack.onStatusChanged(event, status);
    }
}


void VoicePackManager::setJournalPreviousEvent(const std::string& event, const std::string& journalEntry)
{
    _isPriming = true;

    _medicCompliant.update(event, journalEntry);
    const bool compliant = _medicCompliant.isCompliant();

    // Check change of status
    if (compliant != _altaActive) {
        if (!_altaActive) {
            // We are activating ALTA
            std::cout << "[INFO  ] ALTA voicepack activated." << std::endl;
            _medicVoicePack.transferSettings(_standardVoicePack);
        }
        else {
            // We are deactivating ALTA
            std::cout << "[INFO  ] Standard voicepack activated." << std::endl;
            _standardVoicePack.transferSettings(_medicVoicePack);
        }

        _altaActive = compliant;
    }

    if (_altaActive) {
        _medicVoicePack.setJournalPreviousEvent(event, journalEntry);
    }
    else {
        _standardVoicePack.setJournalPreviousEvent(event, journalEntry);
    }

    _isPriming = false;
}


void VoicePackManager::onJournalEvent(const std::string& event, const std::string& journalEntry)
{
    if (event == "Shutdown") {
        _isShutdownState = true;
        std::cout << "[INFO  ] Entering shutdown state" << std::endl;
    }
    else if (event == "LoadGame") {
        _isShutdownState = false;
        std::cout << "[INFO  ] Exiting shutdown state" << std::endl;
    }

    _medicCompliant.update(event, journalEntry);
    const bool compliant = _medicCompliant.isCompliant();

    // Check change of status
    if (compliant != _altaActive) {
        if (!_altaActive) {
            // We are activating ALTA
            std::cout << "[INFO  ] ALTA voicepack activated." << std::endl;
            _medicVoicePack.transferSettings(_standardVoicePack);
            _medicVoicePack.onSpecialEvent(Activating);
        }
        else {
            // We are deactivating ALTA
            std::cout << "[INFO  ] Standard voicepack activated." << std::endl;
            _standardVoicePack.transferSettings(_medicVoicePack);
            _medicVoicePack.onSpecialEvent(Deactivating);
        }

        _altaActive = compliant;
    }

    if (_altaActive) {
        _medicVoicePack.onJournalEvent(event, journalEntry);
    }
    else {
        _standardVoicePack.onJournalEvent(event, journalEntry);
    }
}


void VoicePackManager::playStatusVoiceline(
    Vehicle vehicle,
    StatusEvent event,
    bool status,
    const std::filesystem::path& path)
{
    if (!path.empty() &&
        !_isShutdownState &&
        !_isPriming &&
        _configVoiceStatusActive[vehicle][indexFromStatusEvent(event, status)] == Active) {
        _player.addTrack(path);
    }
}


void VoicePackManager::playJournalVoiceline(
    const std::string& event,
    const std::filesystem::path& path)
{
    if (!path.empty() &&
        !_isShutdownState &&
        !_isPriming &&
        _configVoiceJournalActive[event] == Active) {
        _player.addTrack(path);
    }
}


void VoicePackManager::playSpecialVoiceline(
    SpecialEvent event,
    const std::filesystem::path& path)
{
    if (!path.empty() &&
        !_isShutdownState &&
        !_isPriming &&
        _configVoiceSpecialActive[event] == Active) {
        _player.addTrack(path);
    }
}


void VoicePackManager::setVoiceStatusState(Vehicle vehicle, StatusEvent event, bool statusState, bool active)
{
    const size_t index = 2 * event + (statusState ? 1 : 0);

    if (_configVoiceStatusActive[vehicle][index] != Undefined) {
        _configVoiceStatusActive[vehicle][index] = (active ? Active : Inactive);
    }
}


void VoicePackManager::setVoiceJournalState(const std::string& event, bool active)
{
    if (_configVoiceJournalActive[event] != Undefined) {
        _configVoiceJournalActive[event] = (active ? Active : Inactive);
    }
}


void VoicePackManager::setVoiceSpecialState(SpecialEvent event, bool active)
{
    if (_configVoiceSpecialActive[event] != Undefined) {
        _configVoiceSpecialActive[event] = (active ? Active : Inactive);
    }
}