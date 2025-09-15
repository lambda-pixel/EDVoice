#pragma once

#include <PluginInterface.h>

#include <array>
#include <map>
#include <filesystem>

#include <optional>

#include "AudioPlayer.h"

#include <json.hpp>


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
        _maxShipCargo = other._maxShipCargo;
        _currShipCargo = other._currShipCargo;
        _previousUnderAttack = other._previousUnderAttack;
    }

private:
    void setShipCargo(uint32_t cargo);

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

    std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents> _voiceStatus;
    std::map<std::string, std::filesystem::path> _voiceJournal;
    std::map<std::string, std::filesystem::path> _voiceSpecial;
    AudioPlayer _player;

    uint32_t _maxShipCargo;
    uint32_t _currShipCargo;

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