#pragma once

#include <array>
#include <filesystem>

#include "AudioPlayer.h"
#include "StatusEvent.h"
#include "JournalEvent.h"


class VoicePack
{
public:
    VoicePack(AudioPlayer& p);

    void triggerStatus(StatusEvent::Event event, bool status) const;

    void triggerJournal(JournalEvent::Event event) const;

private:
    std::filesystem::path _basePath;
    std::array<std::wstring, StatusEvent::N_StatusEvents> _voiceStatus;
    std::array<std::wstring, JournalEvent::N_JournalEvent> _voiceJournal;
    AudioPlayer& _player;
};