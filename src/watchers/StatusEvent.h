#pragma once

#include <PluginInterface.h>

#include <optional>
#include <string>


struct StatusEventUtil {
    static const char* toString(StatusEvent ev)
    {
        switch (ev) {
    #define GEN_CASE(name) case name: return #name;
            STATUS_EVENTS(GEN_CASE)
    #undef GEN_CASE
        default: return "Unknown";
        }
    }


    static std::optional<StatusEvent> fromString(const std::string& s)
    {
    #define GEN_IF(name) if (s == #name) return name;
        STATUS_EVENTS(GEN_IF)
    #undef GEN_IF
        return std::nullopt;
    }
};