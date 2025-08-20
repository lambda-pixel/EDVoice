#pragma once

#include <optional>
#include <string>

// List of logged events
#define JOURNAL_EVENTS(X)   \
    X(DockingGranted)       \
    X(DockingDenied)        \
    X(StartJump)

struct JournalEvent {
    // Enum
    enum Event {
    #define GEN_ENUM(name) name,
        JOURNAL_EVENTS(GEN_ENUM)
    #undef GEN_ENUM
        N_JournalEvent
    };


    static const char* toString(Event ev) {
        switch (ev) {
        #define GEN_CASE(name) case name: return #name;
            JOURNAL_EVENTS(GEN_CASE)
        #undef GEN_CASE
        default: return "Unknown";
        }
    }


    static std::optional<Event> fromString(const std::string& s) {
        #define GEN_IF(name) if (s == #name) return name;
                JOURNAL_EVENTS(GEN_IF)
        #undef GEN_IF
            return std::nullopt;
    }
};
