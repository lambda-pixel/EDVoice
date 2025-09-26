#pragma once

#include <iostream>
#include <string>
#include <thread>
#include "AudioPlayer.h"


void STDMETHODCALLTYPE PlayerCallback::OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader)
{
    if (pEventHeader->eEventType == MFP_EVENT_TYPE_PLAYBACK_ENDED) {
        finished = true;
    }
}

STDMETHODIMP PlayerCallback::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    *ppv = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(IMFPMediaPlayerCallback)) {
        *ppv = static_cast<IMFPMediaPlayerCallback*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) PlayerCallback::AddRef()
{
    return ++_refCount;
}

STDMETHODIMP_(ULONG) PlayerCallback::Release()
{
    ULONG ref = --_refCount;
    if (ref == 0) delete this;
    return ref;
}


// ----------------------------------------------------------------------------
// AudioPlayer implementation
// ----------------------------------------------------------------------------

AudioPlayer::AudioPlayer()
    : _eventThread(&AudioPlayer::messageLoop, this)
{
    HRESULT hr;

    hr = MFStartup(MF_VERSION);

    if (FAILED(hr)) {
        throw std::runtime_error("MFStartup failed");
    }

    _playerCallback = new PlayerCallback();

    // Create the player once
    hr = MFPCreateMediaPlayer(NULL, FALSE, 0, _playerCallback, NULL, &_pPlayer);

    if (FAILED(hr)) {
        _playerCallback->Release();
        MFShutdown();
        throw std::runtime_error("MFPCreateMediaPlayer failed");
    }
}


AudioPlayer::~AudioPlayer()
{
    // Signal the thread to stop and post a dummy message to unblock GetMessage
    _stopThread = true;
    PostThreadMessage(GetThreadId(_eventThread.native_handle()), WM_QUIT, 0, 0);

    if (_eventThread.joinable()) {
        _eventThread.join();
    }

    if (_pPlayer) {
        _pPlayer->Shutdown();
        _pPlayer->Release();
    }

    _playerCallback->Release();

    MFShutdown();
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


float AudioPlayer::getVolume() const
{
    return _volume;
}


void AudioPlayer::setVolume(float volume)
{
    _volume = volume;
    _pPlayer->SetVolume(_volume);
}


void AudioPlayer::messageLoop()
{
    HRESULT hr;
    MSG msg;

    while (!_stopThread && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (_playerCallback->finished) {
            // Signal the thread to check for new tracks
            PostThreadMessage(GetCurrentThreadId(), WM_USER + 1, 0, 0);
        }

        if (msg.message == WM_USER + 1) {
            // Go to next track
            if (!_trackQueue.empty()) {
                const std::wstring track = _trackQueue.front();
                _trackQueue.pop();

                _playerCallback->finished = false;
                
                IMFPMediaItem* pMediaItem = nullptr;
                hr = _pPlayer->CreateMediaItemFromURL(track.c_str(), TRUE, NULL, &pMediaItem);

                if (SUCCEEDED(hr) && pMediaItem) {
                    _pPlayer->SetMediaItem(pMediaItem);
                    _pPlayer->Play();
                    pMediaItem->Release();
                }
            }
        }
    }
}

