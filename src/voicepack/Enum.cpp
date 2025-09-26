#include "Enum.h"

// ----------------------------------------------------------------------------
// Vehicule enum and conversion functions
// ----------------------------------------------------------------------------

const char* vehicleToString(Vehicle v)
{
    switch (v) {
#define GEN_CASE(name) case name: return #name;
        ENUM_VEHICULES(GEN_CASE)
#undef GEN_CASE
    default: return "Unknown";
    }
}

std::optional<Vehicle> vehiculeFromString(const std::string& s)
{
#define GEN_IF(name) if (s == #name) return name;
    ENUM_VEHICULES(GEN_IF)
#undef GEN_IF
        return std::nullopt;
}

// ----------------------------------------------------------------------------
// SRV enum and conversion functions
// ----------------------------------------------------------------------------

const char* srvTypeToString(SRVType v)
{
    switch (v) {
#define GEN_CASE(name) case name: return #name;
        ENUM_SRV_TYPES(GEN_CASE)
#undef GEN_CASE
    default: return "Unknown";
    }
}

std::optional<SRVType> srvTypeFromString(const std::string& s)
{
#define GEN_IF(name) if (s == #name) return name;
    ENUM_SRV_TYPES(GEN_IF)
#undef GEN_IF
        return std::nullopt;
}

// ----------------------------------------------------------------------------
// SpecialEvent enum and conversion functions
// ----------------------------------------------------------------------------

const char* specialEventToString(SpecialEvent v)
{
    switch (v) {
#define GEN_CASE(name) case name: return #name;
        ENUM_SPECIAL_EVENTS(GEN_CASE)
#undef GEN_CASE
    default: return "Unknown";
    }
}

std::optional<SpecialEvent> specialEventFromString(const std::string& s)
{
#define GEN_IF(name) if (s == #name) return name;
    ENUM_SPECIAL_EVENTS(GEN_IF)
#undef GEN_IF
        return std::nullopt;
}

// ----------------------------------------------------------------------------
// Status conversion functions
// ----------------------------------------------------------------------------

std::optional<StatusEvent> statusFromString(const std::string& s)
{
#define GEN_IF(name) if (s == #name) return name;
    STATUS_EVENTS(GEN_IF)
#undef GEN_IF
        return std::nullopt;
}

const char* statusToString(StatusEvent ev)
{
    switch (ev) {
#define GEN_CASE(name) case name: return #name;
        STATUS_EVENTS(GEN_CASE)
#undef GEN_CASE
    default: return "Unknown";
    }
}

