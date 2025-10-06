#include "WindowSystem.h"

#include <stdexcept>

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
