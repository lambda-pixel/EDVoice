#pragma once

#include <cstdint>
#include <filesystem>

// TODO: Make a notifier
#include "VoicePack.h"

class StatusWatcher
{
public:
    StatusWatcher(
        const std::filesystem::path& filename,
        const VoicePack& voices
    );

    void update();

private:
    uint32_t getFlags() const;
    void checkUpdatedBits(uint32_t flags);

private:
    bool _ignoreFirst;
    const std::filesystem::path _statusFile;
    uint32_t _previousFlags;

    const VoicePack& _voicePack;
};
