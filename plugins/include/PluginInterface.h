#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* https://elite-journal.readthedocs.io/en/latest/Status%20File.html
0   Docked, (on a landing pad)
1   Landed, (on planet surface)
2   Landing Gear Down
3   Shields Up
4   Supercruise
5   FlightAssist Off
6   Hardpoints Deployed
7   In Wing
8   LightsOn
9   Cargo Scoop Deployed
10  Silent Running,
11  Scooping Fuel
12  Srv Handbrake
13  Srv using Turret view
14  Srv Turret retracted(close to ship)
15  Srv DriveAssist
16  Fsd MassLocked
17  Fsd Charging
18  Fsd Cooldown
19  Low Fuel(< 25 %)
20  Over Heating(> 100 %)
21  Has Lat Long
22  IsInDanger
23  Being Interdicted
24  In MainShip
25  In Fighter
26  In SRV
27  Hud in Analysis mode
28  Night Vision
29  Altitude from Average radius
30  fsdJump
31  srvHighBeam
*/

// List of logged events
#define STATUS_EVENTS(X)            \
    X(Docked)                       \
    X(Landed)                       \
    X(LandingGear_Down)             \
    X(Shields_Up)                   \
    X(Supercruise)                  \
    X(FlightAssist_Off)             \
    X(Hardpoints_Deployed)          \
    X(In_Wing)                      \
    X(LightsOn)                     \
    X(Cargo_Scoop_Deployed)         \
    X(Silent_Running)               \
    X(Scooping_Fuel)                \
    X(Srv_Handbrake)                \
    X(Srv_using_Turret_view)        \
    X(Srv_Turret_retracted)         \
    X(Srv_DriveAssist)              \
    X(Fsd_MassLocked)               \
    X(Fsd_Charging)                 \
    X(Fsd_Cooldown)                 \
    X(Low_Fuel)                     \
    X(Over_Heating)                 \
    X(Has_Lat_Long)                 \
    X(IsInDanger)                   \
    X(Being_Interdicted)            \
    X(In_MainShip)                  \
    X(In_Fighter)                   \
    X(In_SRV)                       \
    X(Hud_in_Analysis_mode)         \
    X(Night_Vision)                 \
    X(Altitude_from_Average_radius) \
    X(fsdJump)                      \
    X(srvHighBeam)

enum StatusEvent {
#define GEN_ENUM(name) name,
    STATUS_EVENTS(GEN_ENUM)
#undef GEN_ENUM
    N_StatusEvents
};

typedef void (*LoadConfigFn)(const char* filepath, void* ctx);
typedef void (*OnStatusChangedFn)(StatusEvent event, int set, void* ctx);
typedef void (*OnJournalEventFn)(const char* event, const char* jsonEntry, void* ctx);

typedef struct {
    LoadConfigFn loadConfig;
    OnStatusChangedFn onStatusChanged;
    OnJournalEventFn onJournalEvent;
    void* ctx;
    char name[32];
    char versionStr[16];
    char author[32];
} PluginCallbacks;

// Each plugin must implement this function to register its callbacks
__declspec(dllexport) void registerPlugin(PluginCallbacks* callbacks);
__declspec(dllexport) void unregisterPlugin();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <cstring>
#endif

// Macro to define plugin registration boilerplate for EventLogger
#define DECLARE_PLUGIN(ClassName, _name, _versionStr, _author) \
    static void loadConfig(const char* filepath, void* ctx) { \
        reinterpret_cast<ClassName*>(ctx)->loadConfig(filepath); \
    } \
    static void onJournalEvent(StatusEvent event, int set, void* ctx) { \
        reinterpret_cast<ClassName*>(ctx)->onStatusChanged(event, set); \
    } \
    static void onStatusChanged(const char* event, const char* jsonEntry, void* ctx) { \
        reinterpret_cast<ClassName*>(ctx)->onJournalEvent(event, jsonEntry); \
    } \
    ClassName* g_plugin = nullptr; \
    extern "C" { \
    __declspec(dllexport) void registerPlugin(PluginCallbacks* callbacks) { \
        g_plugin = new ClassName(); \
        callbacks->loadConfig = loadConfig; \
        callbacks->onStatusChanged = onJournalEvent; \
        callbacks->onJournalEvent = onStatusChanged; \
        callbacks->ctx = g_plugin; \
        std::strncpy(callbacks->name, _name, sizeof(callbacks->name) - 1); \
        std::strncpy(callbacks->versionStr, _versionStr, sizeof(callbacks->versionStr) - 1); \
        std::strncpy(callbacks->author, _author, sizeof(callbacks->author) - 1); \
    } \
    __declspec(dllexport) void unregisterPlugin() { \
        delete g_plugin; \
    } \
    }
