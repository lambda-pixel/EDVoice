#pragma once

#include <array>
#include <map>
#include <filesystem>

#include "AudioPlayer.h"
#include "StatusEvent.h"


class VoicePack
{
public:
    VoicePack(AudioPlayer& p);

    void triggerStatus(StatusEvent::Event event, bool status) const;

    void triggerJournal(const std::string& event) const;

private:
    std::filesystem::path _basePath;
    std::array<std::filesystem::path, StatusEvent::N_StatusEvents> _voiceStatus;
    std::map<std::string, std::filesystem::path> _voiceJournal;
    AudioPlayer& _player;
};