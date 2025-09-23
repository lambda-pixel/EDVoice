#pragma once

#include <PluginInterface.h>

#include <optional>


#define ENUM_VEHICULES(X)            \
    X(Ship)                          \
    X(SRV)                           \
    X(OnFoot)                        \

#define ENUM_SRV_TYPES(X)            \
    X(testbuggy)                     \
    X(combat_multicrew_srv_01)       \

#define ENUM_SPECIAL_EVENTS(X)      \
    X(CargoFull)                    \
    X(CargoEmpty)                   \
    X(CollectPod)                   \
    X(Activating)                   \
    X(Deactivating)                 \
    X(FuelScoopFinished)            \
    X(HullIntegrity_Compromised)    \
    X(HullIntegrity_Critical)       \
    X(AutoPilot_Liftoff)            \
    X(AutoPilot_Touchdown)          \

// ----------------------------------------------------------------------------
// Vehicule enum and conversion functions
// ----------------------------------------------------------------------------

enum Vehicle {
#define GEN_ENUM(name) name,
    ENUM_VEHICULES(GEN_ENUM)
#undef GEN_ENUM
    N_Vehicles
};

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

enum SRVType {
#define GEN_ENUM(name) name,
    ENUM_SRV_TYPES(GEN_ENUM)
#undef GEN_ENUM
    N_SRVTypes
};

const uint32_t SRV_MAX_CARGO[N_SRVTypes] = {
    4, // testbuggy
    2  // combat_multicrew_srv_01
};

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

enum SpecialEvent {
#define GEN_ENUM(name) name,
    ENUM_SPECIAL_EVENTS(GEN_ENUM)
#undef GEN_ENUM
    N_SpecialEvents
};

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

