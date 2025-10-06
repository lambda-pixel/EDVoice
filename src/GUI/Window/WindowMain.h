#pragma once

#include "Window.h"


class WindowMain : public Window
{
public:
    WindowMain(
        WindowSystem* sys,
        const std::string& title,
        const std::filesystem::path& config
    );

    virtual ~WindowMain();

    void minimizeWindow();
    void maximizeRestoreWindow();
    void closeWindow();

    float titleBarHeight() const { return _titlebarHeight; }
    float windowButtonWidth() const { return _buttonWidth; }

    void openVoicePackFileDialog(void* userdata, openedFile callback);

private:
    float _titlebarHeight = 32.f;
    float _buttonWidth = 55.f;
    float _totalButtonWidth = 3 * 55.f;

#ifdef USE_SDL
    void sdlWndProc(SDL_Event& event);
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
    static LRESULT CALLBACK w32WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool w32CompositionEnabled();
    DWORD w32Style();
    void w32SetBorderless(bool borderless);
    bool w32IsMaximized();
    void w32AdjustMaximizedClientRect(RECT& rect);
    LRESULT w32HitTest(POINT cursor) const;
    std::string w32OpenFileName(const char* title, const char* initialDir, const char* filter, bool multiSelect);

    std::wstring _className;
#endif
};