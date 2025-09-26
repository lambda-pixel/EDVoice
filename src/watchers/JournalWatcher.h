#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <thread>

#include <PluginInterface.h>

class JournalListener
{
public:
    virtual void setJournalPreviousEvent(const std::string& event, const std::string& jounralEntry) = 0;
    virtual void onJournalEvent(const std::string& event, const std::string& jounralEntry) = 0;
};


class JournalWatcher
{
public:
    JournalWatcher(const std::filesystem::path& filename);
    virtual ~JournalWatcher();

    void addListener(JournalListener* listener);

    void start();

    void update(const std::filesystem::path& filename);

private:
    void forcedUpdate();

private:
    std::filesystem::path _currJournalPath;
    std::ifstream _currJournalFile;
    std::vector<JournalListener*> _listeners;

    bool _stopForceUpdate;
    std::thread _forcedUpdateThread;
};