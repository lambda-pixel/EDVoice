#include "StatusWatcher.h"

#include <fstream>
#include <json.hpp>
#include <iostream>


StatusWatcher::StatusWatcher(
    const std::filesystem::path& filename)
    : _statusFile(filename)
    , _stopForceUpdate(false)
{
    if (!std::filesystem::exists(filename)) {
        throw std::runtime_error("Cannot find status file: " + filename.string() + ". Did you lanch the game previously?");
    }

    std::wcout << L"[INFO  ] Monitoring: " << filename << std::endl;

    _previousFlags = getFlags();
}


StatusWatcher::~StatusWatcher()
{
    _stopForceUpdate = true;
    
    if (_forcedUpdateThread.joinable()) {
        _forcedUpdateThread.join();
    }
}


void StatusWatcher::addListener(StatusListener* listener)
{
    _listeners.push_back(listener);
}


void StatusWatcher::start()
{
    _forcedUpdateThread = std::thread(&StatusWatcher::forcedUpdate, this);
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

    // First time, when launching the app before the game, just store the flags
    if (_previousFlags == 0) {
        _previousFlags = flags;
        return;
    }

    printChangedBits(flags);

    for (int i_bit = 0; i_bit < 32; i_bit++) {
        const bool prevStatusBit = _previousFlags & (1 << i_bit);
        const bool currStatusBit = flags & (1 << i_bit);

        // Skip it if we already know the status
        if (prevStatusBit != currStatusBit) {
            for (StatusListener* listener : _listeners) {
                listener->onStatusChanged((StatusEvent)i_bit, currStatusBit);
            }
        }
    }

    _previousFlags = flags;
}


void StatusWatcher::printChangedBits(uint32_t flags)
{
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

        // Skip it if we already know the status
        if (prevStatusBit != currStatusBit) {
            std::cout << "x";
        }
        else {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
}


void StatusWatcher::forcedUpdate() {
    // We need to regularly load the file to avoid miss in the WIN32 monitoring
    // in case the file was not flushed to the disk.
    while (!_stopForceUpdate) {
        std::wifstream file(_statusFile, std::ios::in | std::ios::binary);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}