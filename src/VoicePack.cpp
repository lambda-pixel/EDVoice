#include "VoicePack.h"

#include <sstream>


VoicePack::VoicePack(AudioPlayer& p)
    : _player(p)
{
    _basePath = "C:\\Users\\Siegfried\\Desktop\\Bean";

    _voiceStatus[StatusEvent::LandingGear_Down] = L"LandingGear_Down";
    _voiceStatus[StatusEvent::Shields_Up] = L"Shields_Up";
    // Supercruise
    _voiceStatus[StatusEvent::FlightAssist_Off] = L"FlightAssist_Off";
    // Hardpoints_Deployed
    //In_Wing,
    //LightsOn,
    _voiceStatus[StatusEvent::Cargo_Scoop_Deployed] = L"Cargo_Scoop_Deployed";
    //Silent_Running,
    //Scooping_Fuel,
    //Srv_Handbrake,
    //Srv_using_Turret_view,
    //Srv_Turret_retracted,
    //Srv_DriveAssist,
    //Fsd_MassLocked,
    _voiceStatus[StatusEvent::Fsd_Charging] = L"Fsd_Charging";
    //Fsd_Cooldown,
    //Low_Fuel,
    _voiceStatus[StatusEvent::Over_Heating] = L"Over_Heating";
    //Has_Lat_Long,
    //IsInDanger,
    //Being_Interdicted,
    //In_MainShip,
    //In_Fighter,
    //In_SRV,
    //Hud_in_Analysis_mode,
    //Night_Vision,
    //Altitude_from_Average_radius,
    //fsdJump,
    //srvHighBeam

    _voiceJournal[JournalEvent::DockingDenied] = L"DockingDenied.wav";
    _voiceJournal[JournalEvent::DockingGranted] = L"DockingGranted.wav";
    _voiceJournal[JournalEvent::StartJump] = L"StartJump.wav";
}


void VoicePack::triggerStatus(StatusEvent::Event event, bool status) const
{
    if (event == StatusEvent::Fsd_Charging && !status) {
        return;
    }

    if (!_voiceStatus[event].empty()) {
        std::filesystem::path filepath = _basePath / _voiceStatus[event];
        std::wstringstream ss;
        ss << filepath.wstring() << "_" << ((status) ? 1 : 0) << ".wav";
        _player.addTrack(ss.str());
    }
}


void VoicePack::triggerJournal(JournalEvent::Event event) const
{
    if (!_voiceJournal[event].empty()) {
        _player.addTrack(_basePath / _voiceJournal[event]);
    }
}
