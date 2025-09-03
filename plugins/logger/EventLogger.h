#pragma once

#include <PluginInterface.h>

class EventLogger
{
public:
    EventLogger() = default;

    void loadConfig(const char* filepath) {}
    void onStatusChanged(StatusEvent event, int set);
    void onJournalEvent(const char* event, const char* jsonEntry);

private:
    static const char* eventToString(StatusEvent ev);
};

DECLARE_PLUGIN(EventLogger, "EventLogger", "0.1", "Siegfried-Origin")
