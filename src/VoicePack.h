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
    VoicePack(AudioPlayer& p);

    void onStatusChanged(StatusEvent::Event event, bool status) override;

    void onJournalEvent(const std::string& event) override;



private:
    std::filesystem::path _basePath;
    std::array<std::filesystem::path, StatusEvent::N_StatusEvents> _voiceStatus;
    std::map<std::string, std::filesystem::path> _voiceJournal;
    AudioPlayer& _player;
};