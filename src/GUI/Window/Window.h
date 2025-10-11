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
    bool minimized() const { return _minimized; }
    float mainScale() const { return _mainScale; }
    const char* title() const;

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
    void onResize(uint32_t width, uint32_t height);
    void refreshResize();

    const std::filesystem::path _configPath;

#ifdef USE_VULKAN
    VkAdapter _gpuAdapter;
#else
    DX11Adapter _gpuAdapter;
#endif
    bool _gpuInitialized = false;

    WindowSystem* _sys = nullptr;
    ImGuiContext* _imGuiContext = nullptr;
    bool _imGuiInitialized = false;

    // GUI properties
    bool _closed = false;
    bool _minimized = false;
    float _mainScale = 1.f;
    std::string _title;

#ifdef USE_SDL
    virtual void sdlWndProc(SDL_Event& event) = 0;
    SDL_Window* _sdlWindow;
#else
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT w32WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    HWND _hwnd;
    std::wstring _className;
#endif
};
