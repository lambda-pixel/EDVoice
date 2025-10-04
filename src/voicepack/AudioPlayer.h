#pragma once

#include <iostream>
#include <filesystem>
#include <thread>
#include <atomic>

#include <config.h>
#include "AtomicQueue.hpp"

#ifdef USE_SDL_MIXER
    #include <SDL3_mixer/SDL_mixer.h>
#else
    #include <windows.h>
    #include <mfplay.h>
    #include <mfapi.h>
    #include <mferror.h>

    #pragma comment(lib, "mfplat.lib")
    #pragma comment(lib, "mfplay.lib")
    #pragma comment(lib, "mf.lib")

    // ------------------------------------------------------------------------
    // PlayerCallback implementatio
    // ------------------------------------------------------------------------

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
#endif // USE_SDL_MIXER


class AudioPlayer
{
public:
    AudioPlayer();

    ~AudioPlayer();

    void addTrack(const std::filesystem::path& path);

    float getVolume() const;
    void setVolume(float volume);

private:
    std::atomic<bool> _stopThread{ false };
    std::thread _eventThread;

#ifdef USE_SDL_MIXER
    static void SDLCALL trackStoppedCallback(void* userdata, MIX_Track *track);

    MIX_Mixer* _pMixer;
    MIX_Track* _pMainTrack;

    AtomicQueue<MIX_Audio*> _trackQueue;
#else
    void messageLoop();

    IMFPMediaPlayer* _pPlayer = nullptr;
    PlayerCallback* _playerCallback = nullptr;
    AtomicQueue<std::filesystem::path> _trackQueue;
#endif // USE_SDL_MIXER

    float _volume = 1.f;
};