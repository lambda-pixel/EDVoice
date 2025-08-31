#pragma once

#include <PluginInterface.h>

#include <optional>
#include <string>

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
//#define STATUS_EVENTS(X)            \
//    X(Docked)                       \
//    X(Landed)                       \
//    X(LandingGear_Down)             \
//    X(Shields_Up)                   \
//    X(Supercruise)                  \
//    X(FlightAssist_Off)             \
//    X(Hardpoints_Deployed)          \
//    X(In_Wing)                      \
//    X(LightsOn)                     \
//    X(Cargo_Scoop_Deployed)         \
//    X(Silent_Running)               \
//    X(Scooping_Fuel)                \
//    X(Srv_Handbrake)                \
//    X(Srv_using_Turret_view)        \
//    X(Srv_Turret_retracted)         \
//    X(Srv_DriveAssist)              \
//    X(Fsd_MassLocked)               \
//    X(Fsd_Charging)                 \
//    X(Fsd_Cooldown)                 \
//    X(Low_Fuel)                     \
//    X(Over_Heating)                 \
//    X(Has_Lat_Long)                 \
//    X(IsInDanger)                   \
//    X(Being_Interdicted)            \
//    X(In_MainShip)                  \
//    X(In_Fighter)                   \
//    X(In_SRV)                       \
//    X(Hud_in_Analysis_mode)         \
//    X(Night_Vision)                 \
//    X(Altitude_from_Average_radius) \
//    X(fsdJump)                      \
//    X(srvHighBeam)


//struct StatusEvent {
//    // Enum
//    enum Event {
//#define GEN_ENUM(name) name,
//        STATUS_EVENTS(GEN_ENUM)
//#undef GEN_ENUM
//        N_StatusEvents
//    };
//};


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