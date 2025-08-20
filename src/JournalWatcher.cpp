#include "JournalWatcher.h"
#include "JournalEvent.h"

#include <json.hpp>


JournalWatcher::JournalWatcher(
    const std::filesystem::path& filename,
    const VoicePack& voices
)
    : _currJournalPath(filename)
    , _currJournalFile(filename)
    , _voicePack(voices)
{
    _currJournalFile.seekg(0, std::ios::end);

    std::wcout << L"Monitoring: " << _currJournalPath << std::endl;
}


void JournalWatcher::update(const std::filesystem::path& filename)
{
    if (filename != _currJournalPath) {
        // The observed journal changed
        _currJournalFile.close();
        _currJournalFile = std::ifstream(filename);
        _currJournalPath = filename;

        std::wcout << L"Monitoring: " << _currJournalPath << std::endl;
    }

    std::string line;

    while (std::getline(_currJournalFile, line)) {
        auto j = nlohmann::json::parse(line);
        std::cout << "[EVENT] " << j["event"] << std::endl;

        std::optional<JournalEvent::Event> event = JournalEvent::fromString(j["event"]);

        if (event) {
            _voicePack.triggerJournal(event.value());
        }
    }

    // Clear EOF flag
    _currJournalFile.clear();
}