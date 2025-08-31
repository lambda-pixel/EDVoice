#pragma once

#include <array>
#include <map>
#include <filesystem>

#include "AudioPlayer.h"
#include "StatusEvent.h"

#include "JournalWatcher.h"
#include "StatusWatcher.h"


class VoicePack : public StatusListener, public JournalListener
{
public:
    VoicePack(const std::filesystem::path& filepath);

    void onStatusChanged(StatusEvent event, bool status) override;

    void onJournalEvent(const std::string& event, const std::string& journalEntry) override;

private:
    std::array<std::filesystem::path, 2 * StatusEvent::N_StatusEvents> _voiceStatus;
    std::map<std::string, std::filesystem::path> _voiceJournal;
    AudioPlayer _player;
};