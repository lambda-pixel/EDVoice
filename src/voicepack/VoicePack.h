#pragma once

#include <PluginInterface.h>

#include <array>
#include <map>
#include <filesystem>

#include <json.hpp>

#include "AudioPlayer.h"
#include "Enum.h"


class VoicePack
{
public:
    enum TriggerStatus {
        Active,
        Inactive,
        Undefined
    };

    VoicePack();

    void loadConfig(const char* filepath);

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
        _isShutdownState = other._isShutdownState;
        _isPriming = other._isPriming;
    }

    const std::array<std::array<TriggerStatus, 2 * StatusEvent::N_StatusEvents>, N_Vehicles>& getVoiceStatusActive() const { return _voiceStatusActive; }
    
    const std::map<std::string, std::filesystem::path>& getVoiceSpecial() const { return _voiceSpecial; }

    void setVoiceStatusState(Vehicle vehicle, StatusEvent event, bool statusState, bool active);

private:
    void setShipCargo(uint32_t cargo);
    void setSRVCargo(uint32_t cargo);
    void setCurrentVehicle(Vehicle vehicle);

    void playVoiceline(const std::filesystem::path& path);

    static void loadStatusConfig(
        const std::filesystem::path& basePath,
        const nlohmann::json& json,
        std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents>& voiceStatus
    );

    static std::filesystem::path resolvePath(
        const std::filesystem::path& basePath,
        const std::string& file);

    std::array<std::array<TriggerStatus, 2 * StatusEvent::N_StatusEvents>, N_Vehicles> _voiceStatusActive;
    std::array<std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents>, N_Vehicles> _voiceStatusSpecial;

    std::map<std::string, std::filesystem::path> _voiceJournal;
    std::map<std::string, std::filesystem::path> _voiceSpecial;
    AudioPlayer _player;

    Vehicle _currVehicle;

    uint32_t _maxShipFuel;
    uint32_t _maxShipCargo;
    uint32_t _currShipCargo;

    uint32_t _maxSRVCargo;
    uint32_t _currSRVCargo;

    // Uggly hack to prevent multiple "under attack" announcements
    bool _previousUnderAttack;
    bool _isShutdownState = false;

    bool _isPriming = false;
};

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


class Alta
{
public:
    Alta();

    void loadConfig(const char* filepath);
    void onStatusChanged(StatusEvent event, bool status);
    void setJournalPreviousEvent(const std::string& event, const std::string& journalEntry);
    void onJournalEvent(const std::string& event, const std::string& journalEntry);

    // MediCorp specific ALTA voicepack
    MedicCompliant _medicCompliant;
    bool _altaActive = false;

    VoicePack _standardVoicePack;
    VoicePack _altaVoicePack;

    // TODO: this is an ungly hack to prevent reloading ALTA after first load
    bool _altaLoaded;
};

//DECLARE_PLUGIN(Alta, "VoicePack", "0.1", "Siegfried-Origin")

#else

//DECLARE_PLUGIN(VoicePack, "VoicePack", "0.1", "Siegfried-Origin")

#endif // BUILD_MEDICORP