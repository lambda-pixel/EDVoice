#pragma once

#include "WindowSystem.h"

#include <string>
#include <filesystem>
#include <config.h>

#include <imgui.h>

#ifdef USE_SDL
    #include <SDL3/SDL.h>
#else
    #include <windows.h>
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifdef USE_VULKAN
    #include "../Vulkan/VkAdapter.h"
#else
    #include "../DX11/DX11Adapter.h"
#endif


typedef void (*openedFile)(void* userdata, std::string filepath);

class Window
{
protected:
    Window(
        WindowSystem* sys,
        const std::string& title,
        const std::filesystem::path& config
    );
public:
    virtual ~Window();

    virtual void beginFrame();
    virtual void endFrame();

    bool closed() const { return _closed; }

    float getMainScale() const { return _mainScale; }
    bool borderlessWindow() const { return _borderlessWindow; }

    const char* windowTitle() const;

#ifdef USE_VULKAN
    void createVkSurfaceKHR(
        VkInstance instance,
        VkSurfaceKHR* surface, int* width, int* height) const;
#endif

#ifdef USE_SDL
    SDL_Window* handle() const { return _sdlWindow; }
#else
    HWND handle() const { return _hwnd; }
#endif

protected:
    void postInit();
    void onResize(uint32_t width, uint32_t height);
    void refreshResize();

#ifdef USE_VULKAN
    VkAdapter _gpuAdapter;
#else
    DX11Adapter _gpuAdapter;
#endif
    bool _gpuInitialized = false;

    WindowSystem* _sys = nullptr;
    ImGuiContext* _imGuiContext = nullptr;

    bool _imGuiInitialized = false;

    const std::filesystem::path _configPath;

    // GUI properties
    float _mainScale = 1.f;
    bool _borderlessWindow = true;

    std::string _title;

    bool _closed = false;

#ifdef USE_SDL
    virtual void sdlWndProc(SDL_Event& event) = 0;

    SDL_Window* _sdlWindow;
#else
    HWND _hwnd;
#endif
};
