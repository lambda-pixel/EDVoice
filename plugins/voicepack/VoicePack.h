#pragma once

#include <PluginInterface.h>

#include <array>
#include <map>
#include <filesystem>

#include <optional>

#include "AudioPlayer.h"

#include <json.hpp>


enum Vehicle
{
    Ship,
    SRV,
    OnFoot,
    N_Vehicles
};

class VoicePack
{
public:
    VoicePack();

    void loadConfig(const char* filepath);

    void onStatusChanged(StatusEvent event, bool status);

    void onJournalEvent(const std::string& event, const std::string& journalEntry);

    void onSpecialEvent(const std::string& event);

    // Needed to switch between standard and ALTA voicepack
    void transferSettings(const VoicePack& other)
    {
        _currVehicle = other._currVehicle;

        _maxShipCargo = other._maxShipCargo;
        _currShipCargo = other._currShipCargo;

        _maxSRVCargo = other._maxSRVCargo;
        _currSRVCargo = other._currSRVCargo;
    
        _previousUnderAttack = other._previousUnderAttack;
    }

private:
    void setShipCargo(uint32_t cargo);
    void setSRVCargo(uint32_t cargo);
    void setCurrentVehicle(Vehicle vehicle);

    static void loadStatusConfig(
        const std::filesystem::path& basePath,
        const nlohmann::json& json,
        std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents>& voiceStatus
    );

    static std::filesystem::path resolvePath(
        const std::filesystem::path& basePath,
        const std::string& file);

    static std::optional<StatusEvent> statusFromString(const std::string& s)
    {
#define GEN_IF(name) if (s == #name) return name;
        STATUS_EVENTS(GEN_IF)
#undef GEN_IF
       return std::nullopt;
    }

    static const char* statusToString(StatusEvent ev)
    {
        switch (ev) {
#define GEN_CASE(name) case name: return #name;
            STATUS_EVENTS(GEN_CASE)
#undef GEN_CASE
            default: return "Unknown";
        }
    }

    static const char* vehicleToString(Vehicle v)
    {
        // TODO: Make it cleaner
        switch (v) {
        case Ship: return "Ship";
        case SRV: return "SRV";
        case OnFoot: return "OnFoot";
        default: return "Unknown";
        }
    }

    std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents> _voiceStatusCommon;
    std::array<std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents>, N_Vehicles> _voiceStatusSpecial;

    std::map<std::string, std::filesystem::path> _voiceJournal;
    std::map<std::string, std::filesystem::path> _voiceSpecial;
    AudioPlayer _player;

    Vehicle _currVehicle;

    uint32_t _maxShipCargo;
    uint32_t _currShipCargo;

    uint32_t _maxSRVCargo;
    uint32_t _currSRVCargo;

    // Uggly hack to prevent multiple "under attack" announcements
    bool _previousUnderAttack;
};


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

    void onJournalEvent(const std::string& event, const std::string& journalEntry);

    // MediCorp specific ALTA voicepack
    MedicCompliant _medicCompliant;
    bool _altaActive;

    VoicePack _standardVoicePack;
    VoicePack _altaVoicePack;

    // TODO: this is an ungly hack to prevent reloading ALTA after first load
    bool _altaLoaded;
};


DECLARE_PLUGIN(Alta, "VoicePack", "0.1", "Siegfried-Origin")
//DECLARE_PLUGIN(VoicePack, "VoicePack", "0.1", "Siegfried-Origin")