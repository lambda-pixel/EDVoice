#include "AudioPlayer.h"

#include <iostream>
#include <string>
#include <thread>

#include <config.h>

#ifdef USE_SDL_MIXER

#include <SDL3_mixer/SDL_mixer.h>

AudioPlayer::AudioPlayer()
{
    if (!MIX_Init()) {
        throw std::runtime_error(SDL_GetError());
    }

    _pMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (!_pMixer) { throw std::runtime_error(SDL_GetError()); }

    _pMainTrack = MIX_CreateTrack(_pMixer);
    if (!_pMainTrack) { throw std::runtime_error(SDL_GetError()); }

    MIX_SetTrackStoppedCallback(_pMainTrack, trackStoppedCallback, this);
}


AudioPlayer::~AudioPlayer()
{
    MIX_DestroyTrack(_pMainTrack);
    MIX_DestroyMixer(_pMixer);
    MIX_Quit();
}


void AudioPlayer::addTrack(const std::filesystem::path& path)
{
    // TODO: optimization - this is not ideal, the decoding shall be done 
    //       the at voicepack level once instead of decoding again and 
    //       again the same track.
    MIX_Audio* nextVoiceline = MIX_LoadAudio(_pMixer, path.c_str(), true);

    if (!nextVoiceline) {
        std::cerr << "[ERROR ] Could not load track: " << path << " " << SDL_GetError() << std::endl;
        return;
    }

    _trackQueue.push(nextVoiceline);

    // No track are currently playing, we'll start on our own the player
    if (_trackQueue.size() == 1) {
        MIX_Audio* nextAudio = _trackQueue.front();
        MIX_SetTrackAudio(_pMainTrack, nextAudio);

        if (!MIX_PlayTrack(_pMainTrack, 0)) {
            std::cerr << "[ERROR ] Could not start track " << SDL_GetError() << std::endl;
            MIX_DestroyAudio(_trackQueue.front());
            _trackQueue.pop();
        }
    }
}


float AudioPlayer::getVolume() const
{
    return MIX_GetTrackGain(_pMainTrack);
}


void AudioPlayer::setVolume(float volume)
{
    MIX_SetTrackGain(_pMainTrack, volume);
}


void AudioPlayer::trackStoppedCallback(void* userdata, MIX_Track *track)
{
    AudioPlayer* obj = (AudioPlayer*)userdata;

    // Pop the current track, no MIX_DestroyAudio, it is handled by the MIX_Track
    obj->_trackQueue.pop();
    MIX_SetTrackAudio(obj->_pMainTrack, NULL);

    // Check if queue has additional tracks to play else do nothing,
    // addTrack will handle the next track by itself
    if (obj->_trackQueue.size() > 0) {
        MIX_Audio* nextAudio = obj->_trackQueue.front();
        MIX_SetTrackAudio(obj->_pMainTrack, nextAudio);

        if (!MIX_PlayTrack(obj->_pMainTrack, 0)) {
            std::cerr << "[ERROR ] Could not start track " << SDL_GetError() << std::endl;
            MIX_DestroyAudio(obj->_trackQueue.front());
            obj->_trackQueue.pop();
        }
    }
}



#else // USE_SDL_MIXER
void STDMETHODCALLTYPE PlayerCallback::OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader)
{
    if (pEventHeader->eEventType == MFP_EVENT_TYPE_PLAYBACK_ENDED) {
        finished = true;
        PostThreadMessage(_threadId, WM_USER + 1, 0, 0);
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

    _playerCallback = new PlayerCallback(GetThreadId(_eventThread.native_handle()));

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


void AudioPlayer::addTrack(const std::filesystem::path& path)
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

        if (msg.message == WM_USER + 1) {
            // Go to next track
            if (!_trackQueue.empty() && _playerCallback->finished) {
                const std::wstring track = _trackQueue.front();
                _trackQueue.pop();

                IMFPMediaItem* pMediaItem = nullptr;
                hr = _pPlayer->CreateMediaItemFromURL(track.c_str(), TRUE, NULL, &pMediaItem);

                if (SUCCEEDED(hr) && pMediaItem) {
                    _playerCallback->finished = false;
                    _pPlayer->SetMediaItem(pMediaItem);
                    _pPlayer->Play();
                    pMediaItem->Release();
                }
            }
        }
    }
}

#endif // USE_SDL_MIXER