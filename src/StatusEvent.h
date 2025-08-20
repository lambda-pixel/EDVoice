#pragma once


struct StatusEvent {
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
    
    enum Event
    {
        Docked = 0,
        Landed,
        LandingGear_Down,
        Shields_Up,
        Supercruise,
        FlightAssist_Off,
        Hardpoints_Deployed,
        In_Wing,
        LightsOn,
        Cargo_Scoop_Deployed,
        Silent_Running,
        Scooping_Fuel,
        Srv_Handbrake,
        Srv_using_Turret_view,
        Srv_Turret_retracted,
        Srv_DriveAssist,
        Fsd_MassLocked,
        Fsd_Charging,
        Fsd_Cooldown,
        Low_Fuel,
        Over_Heating,
        Has_Lat_Long,
        IsInDanger,
        Being_Interdicted,
        In_MainShip,
        In_Fighter,
        In_SRV,
        Hud_in_Analysis_mode,
        Night_Vision,
        Altitude_from_Average_radius,
        fsdJump,
        srvHighBeam,
        N_StatusEvents
    };
};