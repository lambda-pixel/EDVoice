#pragma once

#include <PluginInterface.h>

#include <array>
#include <map>
#include <filesystem>

#include <optional>

#include "AudioPlayer.h"


class VoicePack
{
public:
    VoicePack();

    void loadConfig(const char* filepath);

    void onStatusChanged(StatusEvent event, bool status);

    void onJournalEvent(const std::string& event, const std::string& journalEntry);

private:
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
    AudioPlayer _player;
};

DECLARE_PLUGIN(VoicePack, "VoicePack", "0.1", "Siegfried-Origin")