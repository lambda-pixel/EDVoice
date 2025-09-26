#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <atomic>


#include <windows.h>
#include <mfplay.h>
#include <mfapi.h>
#include <mferror.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mf.lib")

#include "AtomicQueue.hpp"


class AudioPlayer
{
public:
    AudioPlayer();

    ~AudioPlayer();

    void addTrack(const std::wstring& path);

    float getVolume() const { return _volume; }
    void setVolume(float volume) { _volume = volume; }

private:
    void messageLoop();

    std::atomic<bool> _stopThread{ false };
    std::thread _eventThread;
    AtomicQueue<std::wstring> _trackQueue;

    float _volume = 1.f;
};