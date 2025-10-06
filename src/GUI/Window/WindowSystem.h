#pragma once

#include <config.h>

#include <vector>

#if defined(USE_SDL) || defined(USE_SDL_MIXER)
    #include <SDL3/SDL.h>
#endif

#ifdef USE_SDL
#else
    #include <windows.h>
#endif


class WindowSystem
{
public:
#ifdef USE_SDL
    WindowSystem();
#else
    WindowSystem(HINSTANCE hInstance, int nShowCmd);
#endif // USE_SDL

    virtual ~WindowSystem();

    void getVkInstanceExtensions(std::vector<const char*>& extensions) const;

#ifndef USE_SDL
    HINSTANCE _hInstance;
    int _nShowCmd;
#endif
};
