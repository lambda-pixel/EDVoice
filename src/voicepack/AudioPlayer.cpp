#pragma once

#include <iostream>
#include <string>
#include <thread>
#include "AudioPlayer.h"

// ----------------------------------------------------------------------------
// PlayerCallback implementatio
// ----------------------------------------------------------------------------

class PlayerCallback : public IMFPMediaPlayerCallback {
    std::atomic<ULONG> _refCount{1};
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

// ----------------------------------------------------------------------------
// AudioPlayer implementation
// ----------------------------------------------------------------------------


AudioPlayer::AudioPlayer()
    : _eventThread(&AudioPlayer::messageLoop, this)
{
}


AudioPlayer::~AudioPlayer()
{
    // Signal the thread to stop and post a dummy message to unblock GetMessage
    _stopThread = true;
    PostThreadMessage(GetThreadId(_eventThread.native_handle()), WM_QUIT, 0, 0);

    if (_eventThread.joinable()) {
        _eventThread.join();
    }
}


void AudioPlayer::addTrack(const std::wstring& path)
{
    // Avoid queue twice the same track or overflow the queue
    if (_trackQueue.has(path) || _trackQueue.size() > 4) {
        return;
    }

    _trackQueue.push(path);

    // Signal the thread to check for new tracks
    PostThreadMessage(GetThreadId(_eventThread.native_handle()), WM_USER + 1, 0, 0);
}


void AudioPlayer::messageLoop()
{
    HRESULT hr;
    IMFPMediaPlayer* pPlayer = nullptr;
    PlayerCallback* callback = new PlayerCallback();

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        callback->Release();
        throw std::runtime_error("MFStartup failed");
    }

    MSG msg;
    while (!_stopThread && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (callback->finished) {
            if (pPlayer) {
                pPlayer->Shutdown();
                pPlayer->Release();
                pPlayer = nullptr;
            }
            // Signal the thread to check for new tracks
            PostThreadMessage(GetCurrentThreadId(), WM_USER + 1, 0, 0);
        }

        if (msg.message == WM_USER + 1) {
            // Go to next track
            if (!pPlayer && !_trackQueue.empty()) {
                const std::wstring track = _trackQueue.front();
                _trackQueue.pop();

                hr = MFPCreateMediaPlayer(track.c_str(), FALSE, 0, callback, NULL, &pPlayer);
                if (FAILED(hr)) {
                    callback->Release();
                    MFShutdown();
                    throw std::runtime_error("MFPCreateMediaPlayer failed");
                }

                callback->finished = false;
                
                pPlayer->SetVolume(_volume);
                pPlayer->Play();
            }
        }
    }

    if (pPlayer) {
        pPlayer->Shutdown();
        pPlayer->Release();
    }
    callback->Release();
    MFShutdown();
}

