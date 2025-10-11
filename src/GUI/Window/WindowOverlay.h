#pragma once

#include "Window.h"

#include <config.h>

#include <thread>
#include <atomic>

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

    bool active() { return _active && _shown; }

    void setShown(bool shown);

    void startOverlay(DWORD pid);
    void stopOverlay();

    void processWatch(const std::wstring& processName);

protected:
    static void CALLBACK w32WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG, LONG, DWORD, DWORD);
    virtual LRESULT w32WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    static DWORD getProcessIdByName(const std::wstring& exeName);
    static bool isRealWindow(HWND hwnd);
    static std::vector<HWND> getWindowsForPID(DWORD pid);
    static void positionOverlayOnTarget();

private:
    std::wstring _className;
    bool _active = false;
    bool _shown = true;

    std::thread _processWatcher;
    std::atomic<bool> _stopWatch = false;
};

#endif // !USE_SDL