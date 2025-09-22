#include "EventLogger.h"

#include <iostream>

#include <optional>
#include <string>


void EventLogger::onStatusChanged(StatusEvent event, int set)
{
    std::cout << "[EVENT ] Status change: " << eventToString(event) << " -> " << (set ? "true" : "false") << std::endl;
}


void EventLogger::setJournalPreviousEvent(const char* event, const char* jsonEntry)
{
    std::cout << "[EVENT ] Journal old entry: " << event << std::endl;
}


void EventLogger::onJournalEvent(const char* event, const char* jsonEntry)
{
    std::cout << "[EVENT ] Journal new entry: " << event << std::endl;
}


const char* EventLogger::eventToString(StatusEvent ev)
{
    switch (ev) {
#define GEN_CASE(name) case name: return #name;
        STATUS_EVENTS(GEN_CASE)
#undef GEN_CASE
    default: return "Unknown";
    }
}