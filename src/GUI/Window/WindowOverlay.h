#pragma once

#include "Window.h"

#include <config.h>

// The overlay feature is currently Windows specific
#ifndef USE_SDL

class WindowOverlay: public Window
{
public:
    WindowOverlay(
        WindowSystem* sys,
        const std::string& title,
        const std::filesystem::path& config,
        const std::wstring& processName
    );

    virtual ~WindowOverlay();

protected:
    static void CALLBACK w32WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG, LONG, DWORD, DWORD);
    virtual LRESULT w32WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    virtual DWORD w32Style();
    virtual DWORD dwExStyle();

private:
    HWND _targetWnd;
    std::wstring _className;
};

#endif // !USE_SDL