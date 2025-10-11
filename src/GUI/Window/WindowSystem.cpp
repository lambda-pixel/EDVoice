#include "WindowSystem.h"

#include <stdexcept>
#include <chrono>

#ifdef USE_SDL
    #include <SDL3/SDL_vulkan.h>

    #include <backends/imgui_impl_sdl3.h>
#else
    #include <windowsx.h>
    #include <dwmapi.h>
    #include <commdlg.h>
    #include <codecvt>

    #include <backends/imgui_impl_win32.h>
#endif

#include <backends/imgui_impl_vulkan.h>


WindowSystem::WindowSystem(
#ifdef USE_SDL
)
#else
    HINSTANCE hInstance, int nShowCmd)
    : _hInstance(hInstance)
    , _nShowCmd(nShowCmd)
#endif
{
#if defined(USE_SDL) || defined(USE_SDL_MIXER)
    SDL_InitFlags sdlFlags = 0;
#endif

#ifdef USE_SDL
    sdlFlags |= SDL_INIT_VIDEO | SDL_INIT_GAMEPAD;
#endif

#ifdef USE_SDL_MIXER
    sdlFlags |= SDL_INIT_AUDIO;
 #endif

#if defined(USE_SDL) || defined(USE_SDL_MIXER)
    SDL_Init(sdlFlags);
#endif
}


WindowSystem::~WindowSystem()
{
#if defined(USE_SDL) || defined(USE_SDL_MIXER)
    SDL_Quit();
#endif
}


void WindowSystem::getVkInstanceExtensions(std::vector<const char*>& extensions) const
{
#ifdef USE_SDL
    uint32_t nInstanceExt;
    const char* const* instanceExt = SDL_Vulkan_GetInstanceExtensions(&nInstanceExt);

    for (size_t i = 0; i < nInstanceExt; i++) {
        extensions.push_back(instanceExt[i]);
    }
#else
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back("VK_KHR_win32_surface");
#endif
}


void WindowSystem::collectEvents()
{
    // TODO: make it cleaner, right now, each window collect their
    //       own events during the rendering loop but this blocks
    //       audio when handled by WIN32
    const int TARGET_FPS = 60;
    const int FRAME_DELAY_MS = 1000 / TARGET_FPS;

    auto now = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration<float, std::milli>(now - _lastTime).count();
    _lastTime = now;

#ifdef USE_SDL
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 last = 0;
    double deltaTime = 0;

    if (deltaTime < FRAME_DELAY_MS) {
        SDL_Delay((Uint32)(FRAME_DELAY_MS - delta));
    }
#else
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (delta < FRAME_DELAY_MS) {
        Sleep((DWORD)(FRAME_DELAY_MS - delta));
    }
#endif

    // 
}