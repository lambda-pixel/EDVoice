#pragma once

#include <PluginInterface.h>

#include <array>
#include <map>
#include <filesystem>

#include <json.hpp>

#include "Enum.h"

class VoicePackManager;

class VoicePack
{
public:
    VoicePack(VoicePackManager& voicepackManager);

    void loadConfig(const std::filesystem::path& filepath);

    void onStatusChanged(StatusEvent event, bool status);

    void setJournalPreviousEvent(const std::string& event, const std::string& journalEntry);

    void onJournalEvent(const std::string& event, const std::string& journalEntry);

    void onSpecialEvent(SpecialEvent event);

    // Needed to switch between standard and ALTA voicepack
    void transferSettings(const VoicePack& other)
    {
        _currVehicle = other._currVehicle;

        _maxShipFuel = other._maxShipFuel;
        _maxShipCargo = other._maxShipCargo;
        _currShipCargo = other._currShipCargo;

        _maxSRVCargo = other._maxSRVCargo;
        _currSRVCargo = other._currSRVCargo;

        _previousUnderAttack = other._previousUnderAttack;
        _previousLaunchDrone = other._previousLaunchDrone;
        _previousEjectCargo = other._previousEjectCargo;

        _isShutdownState = other._isShutdownState;
        _isPriming = other._isPriming;
    }

    std::array<std::array<VoiceTriggerStatus, 2 * StatusEvent::N_StatusEvents>, N_Vehicles>& getVoiceStatusActive() { return _voiceStatusActive; }
    std::map<std::string, VoiceTriggerStatus>& getVoiceJournalActive() { return _voiceJournalActive; }
    std::array<VoiceTriggerStatus, N_SpecialEvents>& getVoiceSpecialActive() { return _voiceSpecialActive; }

    void setVoiceStatusState(Vehicle vehicle, StatusEvent event, bool statusState, bool active);
    void setVoiceJournalState(const std::string& event, bool active);
    void setVoiceSpecialState(SpecialEvent event, bool active);

    const std::filesystem::path& getVoicePackPath() const { return _configPath; }

private:
    void setShipCargo(uint32_t cargo);
    void setSRVCargo(uint32_t cargo);
    void setCurrentVehicle(Vehicle vehicle);

    static void loadStatusConfig(
        const std::filesystem::path& basePath,
        const nlohmann::json& json,
        std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents>& voiceStatus
    );

    std::filesystem::path _configPath;
    VoicePackManager& _voicePackManager;

    std::array<std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents>, N_Vehicles> _voiceStatus;
    std::map<std::string, std::filesystem::path> _voiceJournal;
    std::array<std::filesystem::path, N_SpecialEvents> _voiceSpecial;

    std::array<std::array<VoiceTriggerStatus, 2 * StatusEvent::N_StatusEvents>, N_Vehicles> _voiceStatusActive;
    std::map<std::string, VoiceTriggerStatus> _voiceJournalActive;
    std::array<VoiceTriggerStatus, N_SpecialEvents> _voiceSpecialActive;

    Vehicle _currVehicle = Vehicle::Ship;

    uint32_t _maxShipFuel = 0;
    uint32_t _maxShipCargo = 0;
    uint32_t _currShipCargo = 0;

    uint32_t _maxSRVCargo = 0;
    uint32_t _currSRVCargo = 0;

    // Uggly hack to prevent multiple "under attack" announcements
    bool _previousUnderAttack;
    bool _previousLaunchDrone = false;
    bool _previousEjectCargo = false;

    bool _isShutdownState = false;
    bool _isPriming = false;
};


