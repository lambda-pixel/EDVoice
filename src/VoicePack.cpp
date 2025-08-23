#include "VoicePack.h"

#include <sstream>
#include <json.hpp>

VoicePack::VoicePack(const std::filesystem::path& filepath, AudioPlayer& p)
    : _player(p)
{
    if (!std::filesystem::exists(filepath)) {
        throw std::runtime_error("VoicePack: Cannot find configuration file: " + filepath.string());
    }

    std::filesystem::path basePath;

    if (filepath.is_absolute()) {
        basePath = filepath.parent_path();
    }
    else {
        basePath = std::filesystem::current_path() / filepath.parent_path();
    }

    auto resolvePath = [basePath](const std::string& file) -> std::filesystem::path {
        std::filesystem::path p(file);
        return p.is_absolute() ? p : (basePath / p);
    };

    try {
        std::ifstream file(filepath);
        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        nlohmann::json json = nlohmann::json::parse(fileContent);

        // Parse status
        if (json.contains("status")) {
            for (auto& st : json["status"].items()) {
                const std::optional<StatusEvent::Event> status = StatusEvent::fromString(st.key());

                if (!status || status == StatusEvent::N_StatusEvents) {
                    std::cout << "[WARN  ] Unknown status event: " << st.key() << "\n";
                    continue;
                }

                for (auto& statusEntry : st.value().items()) {
                    if (statusEntry.key() == "true") {
                        _voiceStatus[2 * status.value() + 1] = resolvePath(statusEntry.value().get<std::string>());
                    }
                    else if (statusEntry.key() == "false") {
                        _voiceStatus[2 * status.value() + 0] = resolvePath(statusEntry.value().get<std::string>());
                    }
                    else {
                        std::cout << "[WARN  ] Unknown status key: " << statusEntry.key() << "\n";
                    }
                }
            }
        }

        // Parse journal
        if (json.contains("event")) {
            for (auto& je : json["event"].items()) {
                _voiceJournal[je.key()] = resolvePath(je.value().get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERR] JSON: " << e.what() << "\n";
    }

    // Log missing files and remove them from the list
    for (size_t iEvent = 0; iEvent < StatusEvent::N_StatusEvents; iEvent++) {
        const std::string eventName = StatusEvent::toString((StatusEvent::Event)iEvent);

        for (size_t i = 0; i < 2; i++) {
            const std::string state = (i == 0) ? "false" : "true";
            const size_t index = 2 * iEvent + i;

            if (!_voiceStatus[index].empty() && !std::filesystem::exists(_voiceStatus[index])) {
                std::cout << "[ERR   ] Missing file for status '" << eventName << "' (" << state << "): " << _voiceStatus[index] << std::endl;
                _voiceStatus[index].clear();
            }
        }
    }

    for (auto& [event, path] : _voiceJournal) {
        if (!path.empty() && !std::filesystem::exists(path)) {
            std::cerr << "[ERR   ] Missing file for event '" << event << "': " << path << std::endl;
            path.clear();
        }
    }
}


void VoicePack::onStatusChanged(StatusEvent::Event event, bool status)
{
    if (event >= StatusEvent::N_StatusEvents) {
        return;
    }

    const size_t index = 2 * event + (status ? 1 : 0);

    if (!_voiceStatus[index].empty()) {
        _player.addTrack(_voiceStatus[index]);
    }
}


void VoicePack::onJournalEvent(const std::string& event, const std::string& journalEntry)
{
    auto it = _voiceJournal.find(event);

    if (it != _voiceJournal.end()) {
        _player.addTrack(it->second);
    }
}
