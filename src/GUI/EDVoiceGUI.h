#pragma once

#ifdef _WIN32
    #include <windows.h>
#else
    #include <SDL3/SDL.h>
    #include <SDL3/SDL_vulkan.h>
#endif

#include "Vulkan/VkAdapter.h"

#include <filesystem>
#include <thread>
#include "../EDVoiceApp.h"

class EDVoiceGUI
{
public:

#ifdef _WIN32
    EDVoiceGUI(
        const std::filesystem::path& exec_path,
        const std::filesystem::path& config,
        HINSTANCE hInstance, int nShowCmd);
#else
    EDVoiceGUI(
        const std::filesystem::path& exec_path,
        const std::filesystem::path& config);
#endif

    ~EDVoiceGUI();

    void run();

private:
    void resize();

    void beginMainWindow();
    void endMainWindow();

    void voicePackGUI();

    void voicePackStatusGUI();
    void voicePackJourmalEventGUI();
    void voicePackSpecialEventGUI();

    void voicePackOpenDialogGUI();

    static const char* prettyPrintStatusState(StatusEvent status, bool activated);
    static const char* prettyPrintVehicle(Vehicle vehicle);
    static const char* prettyPrintSpecialEvent(SpecialEvent event);

    EDVoiceApp* _app;
    VkAdapter _vkAdapter;

    bool _imGuiInitialized = false;
    float _mainScale = 1.f;
    bool _borderlessWindow = true;
    float _titlebarHeight = 32.f;
    float _totalButtonWidth = 3 * 32.f;

    bool _hasError = false;
    std::string _logErrStr;

    std::filesystem::path _imGuiIniPath;

#ifdef _WIN32
    // WIN32 stuff for working with borderless windows
    // see https://github.com/melak47/BorderlessWindow/tree/main
    static LRESULT CALLBACK w32WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool w32CompositionEnabled();
    DWORD w32Style();
    void w32CreateWindow(int nShowCmd);
    void w32SetBorderless(bool borderless);
    bool w32IsMaximized();
    void w32AdjustMaximizedClientRect(RECT& rect);
    LRESULT w32HitTest(POINT cursor) const;
    std::string w32OpenFileName(const char* title, const char* initialDir, const char* filter, bool multiSelect);

    HWND _hwnd;
    HINSTANCE _hInstance;
#else
    static void SDLCALL sdlCallbackOpenFile(void* userdata, const char* const* filelist, int filter);

    SDL_Window* _sdlWindow;
#endif
};