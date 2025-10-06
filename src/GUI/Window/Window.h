#pragma once

#include "WindowSystem.h"

#include <string>
#include <filesystem>
#include <config.h>

#include <imgui.h>

#ifdef USE_SDL
    #include <SDL3/SDL_vulkan.h>
#else
    #include <windows.h>
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "../Vulkan/VkAdapter.h"

typedef void (*openedFile)(void* userdata, std::string filepath);

class Window
{
public:
    Window(
        WindowSystem* sys,
        const std::string& title,
        const std::filesystem::path& config
    );

    virtual ~Window();

    void onResize(uint32_t width, uint32_t height);

    void beginFrame();
    void endFrame();

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
        VkSurfaceKHR* surface, int* width, int* height) const;

private:
    void refreshResize();

private:
    VkAdapter _vkAdapter;
    WindowSystem* _sys = nullptr;
    ImGuiContext* _imGuiContext = nullptr;

    bool _imGuiInitialized = false;

    const std::filesystem::path _configPath;

    // GUI properties
    float _mainScale = 1.f;
    bool _borderlessWindow = true;

    float _titlebarHeight = 32.f;
    float _buttonWidth = 55.f;
    float _totalButtonWidth = 3 * 55.f;
    std::string _title;

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

