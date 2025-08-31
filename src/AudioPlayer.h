#pragma once

#include <iostream>
#include <string>
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

class AudioPlayer {
public:
    AudioPlayer()
    {
    }

    void addTrack(const std::wstring& path) {
        // Workarround for now :
        // TODO: maintain a queue which plays the track one after the other
        if (!PlaySoundW(path.c_str(), NULL, SND_FILENAME | SND_ASYNC)) {
            std::wcerr << "Could not locate sound file: " << path << std::endl;
        }
    }
};