#pragma once

#include <iostream>
#include <filesystem>
#include <thread>
#include <atomic>


#ifdef _WIN32
    #include <windows.h>
    #include <mfplay.h>
    #include <mfapi.h>
    #include <mferror.h>

    #pragma comment(lib, "mfplat.lib")
    #pragma comment(lib, "mfplay.lib")
    #pragma comment(lib, "mf.lib")
#endif

#include "AtomicQueue.hpp"


// ----------------------------------------------------------------------------
// PlayerCallback implementatio
// ----------------------------------------------------------------------------
#ifdef _WIN32
class PlayerCallback : public IMFPMediaPlayerCallback {
    std::atomic<ULONG> _refCount{ 1 };
public:

    PlayerCallback(DWORD threadId)
        : _threadId(threadId) {
    }

    bool finished = true;
    DWORD _threadId;

    void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader) override;
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
};
#endif


class AudioPlayer
{
public:
    AudioPlayer();

    ~AudioPlayer();

    void addTrack(const std::filesystem::path& path);

    float getVolume() const;
    void setVolume(float volume);

private:
    void messageLoop();

    std::atomic<bool> _stopThread{ false };
    std::thread _eventThread;
    AtomicQueue<std::filesystem::path> _trackQueue;

#ifdef _WIN32
    IMFPMediaPlayer* _pPlayer = nullptr;
    PlayerCallback* _playerCallback = nullptr;
#endif

    float _volume = 1.f;
};