#pragma once

#include <PluginInterface.h>

class EventLogger
{
public:
    EventLogger() = default;

    void onStatusChanged(StatusEvent event, int set);
    void onJournalEvent(const char* event, const char* jsonEntry);

private:
    static const char* eventToString(StatusEvent ev);
};

DECLARE_PLUGIN(EventLogger)
