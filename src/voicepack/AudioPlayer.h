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

    void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader) override {
        if (pEventHeader->eEventType == MFP_EVENT_TYPE_PLAYBACK_ENDED) {
            finished = true;
        }
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(IMFPMediaPlayerCallback)) {
            *ppv = static_cast<IMFPMediaPlayerCallback*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override {
        return ++_refCount;
    }

    STDMETHODIMP_(ULONG) Release() override {
        ULONG ref = --_refCount;
        if (ref == 0) delete this;
        return ref;
    }
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