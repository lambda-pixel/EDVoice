#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include <PluginInterface.h>

class JournalListener
{
public:
    virtual void onJournalEvent(const std::string& event, const std::string& jounralEntry) = 0;
};


class JournalWatcher
{
public:
    JournalWatcher(const std::filesystem::path& filename);

    void addListener(JournalListener* listener);

    void update(const std::filesystem::path& filename);

private:
    std::filesystem::path _currJournalPath;
    std::ifstream _currJournalFile;
    std::vector<JournalListener*> _listeners;
};