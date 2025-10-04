#pragma once

#include <string>
#include <config.h>

#include <imgui.h>

#if defined(USE_SDL) || defined(USE_SDL_MIXER)
    #include <SDL3/SDL.h>
#endif

#ifdef USE_SDL
    #include <SDL3/SDL_vulkan.h>
#else
    #include <windows.h>
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "Vulkan/VkAdapter.h"

typedef void (*openedFile)(void* userdata, std::string filepath);

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
    // TODO: create vulkan context first
};


class Window
{
public:
    Window(WindowSystem* sys, VkAdapter* vkAdapter, const std::string& title);
    virtual ~Window();

    void render();

    void minimizeWindow();
    void maximizeRestoreWindow();
    void closeWindow();

    void openVoicePackFileDialog(void* userdata, openedFile callback);

    bool quit() const { return _quit; }

    float getMainScale() const { return _mainScale; }
    bool borderlessWindow() const { return _borderlessWindow; }

    float titleBarHeight() const { return _titlebarHeight; }
    float windowButtonWidth() const { return _buttonWidth; }

    const char* windowTitle() const;

    void createVkSurfaceKHR(
        VkInstance instance,
        VkSurfaceKHR* surface, int *width, int *height) const;

private:
    VkAdapter* _vkAdapter = nullptr;
    WindowSystem* _sys;

    ImGuiContext* _imGuiContext;

    float _mainScale = 1.f;
    bool _borderlessWindow = true;
    // TODO: make scale aware
    float _titlebarHeight = 32.f;
    float _buttonWidth = 55.f;
    float _totalButtonWidth = 3 * 55.f;

    bool _quit = false;

#ifdef USE_SDL
    void sdlWndProc(SDL_Event& event);
    static SDL_HitTestResult SDLCALL sdlHitTest(SDL_Window* win, const SDL_Point* area, void* data);

    struct OpenFileCbData {
        void* userdata;
        openedFile callback;
    };

    static void SDLCALL sdlCallbackOpenFile(void* userdata, const char* const* filelist, int filter);

    SDL_Window* _sdlWindow;
    bool _isMaximized = false;
#else
    // WIN32 stuff for working with borderless windows
    // see https://github.com/melak47/BorderlessWindow/tree/main
    static LRESULT CALLBACK w32WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool w32CompositionEnabled();
    DWORD w32Style();
    void w32SetBorderless(bool borderless);
    bool w32IsMaximized();
    void w32AdjustMaximizedClientRect(RECT& rect);
    LRESULT w32HitTest(POINT cursor) const;
    std::string w32OpenFileName(const char* title, const char* initialDir, const char* filter, bool multiSelect);

    HWND _hwnd;
    std::wstring _className;
#endif
};

