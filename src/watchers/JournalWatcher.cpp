#include "JournalWatcher.h"

#include <iostream>
#include <json.hpp>


JournalWatcher::JournalWatcher(const std::filesystem::path& filename)
    : _currJournalPath(filename)
    , _currJournalFile(filename)
    , _stopForceUpdate(false)
{
    std::wcout << L"[INFO  ] Monitoring: " << _currJournalPath << std::endl;
}


JournalWatcher::~JournalWatcher()
{
    _stopForceUpdate = true;
    _forcedUpdateThread.join();
}


void JournalWatcher::addListener(JournalListener* listener)
{
    _listeners.push_back(listener);
}


void JournalWatcher::start()
{
    // Prime the listeners with existing entries
    std::string line;

    while (std::getline(_currJournalFile, line)) {
        auto j = nlohmann::json::parse(line);

        for (JournalListener* listener : _listeners) {
            listener->setJournalPreviousEvent(j["event"], line);
        }
    }

    _forcedUpdateThread = std::thread(&JournalWatcher::forcedUpdate, this);
}


void JournalWatcher::update(const std::filesystem::path& filename)
{
    if (filename != _currJournalPath) {
        // The observed journal changed
        _currJournalFile.close();
        _currJournalFile = std::ifstream(filename);
        _currJournalPath = filename;

        std::wcout << L"[INFO  ] Monitoring: " << _currJournalPath << std::endl;
    }

    std::string line;

    while (std::getline(_currJournalFile, line)) {
        auto j = nlohmann::json::parse(line);

        for (JournalListener* listener : _listeners) {
            listener->onJournalEvent(j["event"], line);
        }
    }

    // Clear EOF flag
    _currJournalFile.clear();
}


void JournalWatcher::forcedUpdate() {
    // We need to regularly load the file to avoid miss in the WIN32 monitoring
    // in case the file was not flushed to the disk.
    while (!_stopForceUpdate) {
        std::wifstream file(_currJournalPath, std::ios::in | std::ios::binary);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}