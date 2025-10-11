#pragma once

#include "Window.h"

typedef void (*openedFile)(void* userdata, std::string filepath);

class WindowBorderless : public Window
{
public:
    WindowBorderless(
        WindowSystem* sys,
        const std::string& title,
        const std::filesystem::path& config
    );

    virtual ~WindowBorderless();

    void minimizeWindow();
    void maximizeRestoreWindow();
    void closeWindow();

    float titleBarHeight() const { return _titlebarHeight; }
    float windowButtonWidth() const { return _buttonWidth; }
    bool borderless() const { return _borderless; }

    void openVoicePackFileDialog(void* userdata, openedFile callback);

protected:
    float _titlebarHeight = 32.f;
    float _buttonWidth = 55.f;
    float _totalButtonWidth = 3 * 55.f;
    bool _borderless = true;

#ifdef USE_SDL
    virtual void sdlWndProc(SDL_Event& event);

    static SDL_HitTestResult SDLCALL sdlHitTest(SDL_Window* win, const SDL_Point* area, void* data);

    struct OpenFileCbData {
        void* userdata;
        openedFile callback;
    };

    static void SDLCALL sdlCallbackOpenFile(void* userdata, const char* const* filelist, int filter);

    bool _isMaximized = false;
#else
    // WIN32 stuff for working with borderless windows
    // see https://github.com/melak47/BorderlessWindow/tree/main
    virtual LRESULT w32WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    virtual DWORD w32Style();

    static bool w32CompositionEnabled();

    void w32SetBorderless(bool borderless);
    bool w32IsMaximized();
    void w32AdjustMaximizedClientRect(RECT& rect);
    LRESULT w32HitTest(POINT cursor) const;
    std::string w32OpenFileName(const char* title, const char* initialDir, const char* filter, bool multiSelect);
#endif
};