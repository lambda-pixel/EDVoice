#pragma once

#include "JournalWatcher.h"
#include "StatusWatcher.h"

class EventLogger : public StatusListener, public JournalListener
{
public:
    EventLogger() = default;
    void onStatusChanged(StatusEvent::Event event, bool status) override;
    void onJournalEvent(const std::string& event, const std::string& journalEntry) override;
};