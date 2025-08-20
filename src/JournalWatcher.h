#pragma once

#include <filesystem>
#include <fstream>

#include "VoicePack.h"


class JournalWatcher
{
public:
    JournalWatcher(
        const std::filesystem::path& filename,
        const VoicePack& voices
    );

    void update(const std::filesystem::path& filename);

private:
    std::filesystem::path _currJournalPath;
    std::ifstream _currJournalFile;
    const VoicePack& _voicePack;
};