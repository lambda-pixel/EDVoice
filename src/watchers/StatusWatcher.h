#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>
#include <thread>
#include <atomic>

#include "StatusEvent.h"


class StatusListener
{
public:
    virtual void onStatusChanged(StatusEvent event, bool set) = 0;
};


class StatusWatcher
{
public:
    StatusWatcher(const std::filesystem::path& filename);
    virtual ~StatusWatcher();
    
    void addListener(StatusListener* listener);

    void start();

    void update();

private:
    uint32_t getFlags() const;

    void checkUpdatedBits(uint32_t flags);

    void printChangedBits(uint32_t flags);

private:
    void forcedUpdate();

private:
    const std::filesystem::path _statusFile;
    uint32_t _previousFlags;

    std::vector<StatusListener*> _listeners;

    std::atomic<bool> _stopForceUpdate;
    std::thread _forcedUpdateThread;
};
