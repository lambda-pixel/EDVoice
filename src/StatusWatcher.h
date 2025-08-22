#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "StatusEvent.h"


class StatusListener
{
public:
    virtual void onStatusChanged(StatusEvent::Event event, bool set) = 0;
};


class StatusWatcher
{
public:
    StatusWatcher(const std::filesystem::path& filename);

    void addListener(StatusListener* listener);

    void update();

private:
    uint32_t getFlags() const;

    void checkUpdatedBits(uint32_t flags);

    void printChangedBits(uint32_t flags);

private:
    const std::filesystem::path _statusFile;
    uint32_t _previousFlags;

    std::vector<StatusListener*> _listeners;
};
