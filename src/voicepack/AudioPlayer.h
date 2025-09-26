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


// ----------------------------------------------------------------------------
// PlayerCallback implementatio
// ----------------------------------------------------------------------------

class PlayerCallback : public IMFPMediaPlayerCallback {
    std::atomic<ULONG> _refCount{ 1 };
public:
    bool finished = false;

    void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader) override;
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
};



class AudioPlayer
{
public:
    AudioPlayer();

    ~AudioPlayer();

    void addTrack(const std::wstring& path);

    float getVolume() const;
    void setVolume(float volume);

private:
    void messageLoop();

    std::atomic<bool> _stopThread{ false };
    std::thread _eventThread;
    AtomicQueue<std::wstring> _trackQueue;

// win32 stuff
    IMFPMediaPlayer* _pPlayer = nullptr;
    PlayerCallback* _playerCallback = nullptr;

    float _volume = 1.f;
};