#include "EventLogger.h"

#include <iostream>


void EventLogger::onStatusChanged(StatusEvent::Event event, bool status)
{
    std::cout << "[EVENT ] Status change: " << StatusEvent::toString(event) << " -> " << (status ? "true" : "false") << std::endl;
}


void EventLogger::onJournalEvent(const std::string& event, const std::string& journalEntry)
{
    std::cout << "[EVENT ] Journal entry: " << event << std::endl;
}