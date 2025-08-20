#include "StatusWatcher.h"
#include "StatusEvent.h"

#include <fstream>
#include <json.hpp>
#include <iostream>


StatusWatcher::StatusWatcher(
    const std::filesystem::path& filename,
    const VoicePack& voice)
    : _ignoreFirst(true)
    , _statusFile(filename)
    , _voicePack(voice)
{
    _previousFlags = getFlags();

    std::wcout << L"[STATUS] Monitoring: " << filename << std::endl;
}


void StatusWatcher::update()
{
    checkUpdatedBits(getFlags());
}


uint32_t StatusWatcher::getFlags() const
{
    std::ifstream file(_statusFile, std::ios::in);
    nlohmann::json j;
    std::string line;
    uint32_t currentFlags = 0;

    if (std::getline(file, line)) {
        try {
            auto j = nlohmann::json::parse(line);

            if (j["Flags"].is_number_integer()) {
                currentFlags = j["Flags"].get<uint32_t>();
            }
        }
        catch (std::exception& e) {
            std::cerr << "ERR JSON: " << e.what() << "\n";
        }
    }

    return currentFlags;
}


void StatusWatcher::checkUpdatedBits(uint32_t flags)
{
    // ignore 0
    if (!flags || flags == _previousFlags) {
        return;
    }

    //*

    std::cout << "[STATUS] Previous flags ";

    for (int i_bit = 0; i_bit < 32; i_bit++) {
        const bool prevStatusBit = _previousFlags & (1 << i_bit);
        if (prevStatusBit) {
            std::cout << 1;
        }
        else {
            std::cout << 0;
        }
    }
    std::cout << std::endl;

    std::cout << "[STATUS] Current flags  ";

    for (int i_bit = 0; i_bit < 32; i_bit++) {
        const bool currStatusBit = flags & (1 << i_bit);
        if (currStatusBit) {
            std::cout << 1;
        }
        else {
            std::cout << 0;
        }
    }
    std::cout << std::endl;

    std::cout << "[STATUS]                ";

    for (int i_bit = 0; i_bit < 32; i_bit++) {
        const bool prevStatusBit = _previousFlags & (1 << i_bit);
        const bool currStatusBit = flags & (1 << i_bit);

        // Skip it, we already know the status
        if (prevStatusBit != currStatusBit) {
            std::cout << "x";
        }
        else {
            std::cout << " ";
        }
    }
    std::cout << std::endl;

    //*/

    for (int i_bit = 0; i_bit < 32; i_bit++) {
        const bool prevStatusBit = _previousFlags & (1 << i_bit);
        const bool currStatusBit = flags & (1 << i_bit);

        // Skip it, we already know the status
        if (prevStatusBit != currStatusBit) {
            _voicePack.triggerStatus((StatusEvent::Event)i_bit, currStatusBit);
        }
    }

    //std::cout << std::endl;

    _previousFlags = flags;
}
