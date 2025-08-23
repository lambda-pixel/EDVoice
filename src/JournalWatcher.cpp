#include "JournalWatcher.h"

#include <iostream>
#include <json.hpp>


JournalWatcher::JournalWatcher(const std::filesystem::path& filename)
    : _currJournalPath(filename)
    , _currJournalFile(filename)
{
    std::wcout << L"[INFO  ] Monitoring: " << _currJournalPath << std::endl;
    
    _currJournalFile.seekg(0, std::ios::end);
}


void JournalWatcher::addListener(JournalListener* listener)
{
    _listeners.push_back(listener);
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