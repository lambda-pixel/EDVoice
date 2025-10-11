#include "WindowOverlay.h"

#ifndef USE_SDL

#include <windowsx.h>
#include <dwmapi.h>
#include <commdlg.h>
#include <codecvt>
#include <tlhelp32.h>

#include "inter.cpp"

// TODO: Temporary
HWND g_overlayWnd = NULL;
HWND g_targetWnd = NULL;


WindowOverlay::WindowOverlay(
    WindowSystem* sys,
    const std::string& title,
    const std::filesystem::path& config,
    const std::wstring& processName)
    : Window(sys, title, config)
{
    // TODO: find a cleaner solution than global vars
    // Already hooked once
    if (g_overlayWnd) {
        throw std::runtime_error("Cannot hook two overlays");
    }

    g_overlayWnd = _hwnd;

    // Global hook
    HWINEVENTHOOK hook = SetWinEventHook(
        EVENT_OBJECT_CREATE,
        EVENT_OBJECT_LOCATIONCHANGE,
        NULL,
        w32WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    if (!hook) {
        throw std::runtime_error("SetWinEventHook failed");
    }

    LONG_PTR exStyle = WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    //LONG_PTR exStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    SetWindowLongPtr(_hwnd, GWL_EXSTYLE, exStyle);

    LONG_PTR styleWin = WS_POPUP;
    SetWindowLongPtr(_hwnd, GWL_STYLE, styleWin);

    SetLayeredWindowAttributes(_hwnd, 0, 255, LWA_ALPHA);
    const MARGINS margin = { -1, 0, 0, 0 };
    DwmExtendFrameIntoClientArea(_hwnd, &margin);

    SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    ::UpdateWindow(_hwnd);

    setShown(_shown);

    _processWatcher = std::thread(&WindowOverlay::processWatch, this, processName);
}


WindowOverlay::~WindowOverlay()
{
    _stopWatch = true;

    if (_processWatcher.joinable()) {
        _processWatcher.join();
    }
}


void WindowOverlay::setShown(bool shown)
{
    _shown = shown;
    ::ShowWindow(_hwnd, active() ? SW_SHOW : SW_MINIMIZE);
    ::UpdateWindow(_hwnd);
}


void WindowOverlay::startOverlay(DWORD pid)
{
    auto windows = getWindowsForPID(pid);

    if (!windows.empty()) {
        g_targetWnd = windows[0];

        positionOverlayOnTarget();
        _active = true;

        ::ShowWindow(_hwnd, active() ? SW_SHOW : SW_MINIMIZE);
        ::UpdateWindow(_hwnd);
    }
}


void WindowOverlay::stopOverlay()
{
    g_targetWnd = nullptr;
    ::ShowWindow(_hwnd, SW_HIDE);
    _active = false;

    ::ShowWindow(_hwnd, SW_MINIMIZE);
    ::UpdateWindow(_hwnd);
}


void WindowOverlay::processWatch(const std::wstring& processName)
{
    DWORD lastPid = 0;

    while (!_stopWatch) {
        DWORD pid = getProcessIdByName(processName);
        if (pid == 0 && lastPid != 0) {
            stopOverlay();
            lastPid = pid;
        }
        else if (pid != 0 && lastPid == 0) {
            startOverlay(pid);
            lastPid = pid;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    if (lastPid != 0) {
        stopOverlay();
    }
}


LRESULT WindowOverlay::w32WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    return ::DefWindowProcW(_hwnd, msg, wParam, lParam);
}


void CALLBACK WindowOverlay::w32WinEventProc(
    HWINEVENTHOOK, DWORD event, HWND hwnd,
    LONG, LONG, DWORD, DWORD)
{
    if (g_targetWnd == nullptr || hwnd != g_targetWnd) { return; }

    if (event == EVENT_OBJECT_LOCATIONCHANGE || event == EVENT_OBJECT_CREATE) {
        positionOverlayOnTarget();
    }
}


DWORD WindowOverlay::getProcessIdByName(const std::wstring& exeName)
{
    DWORD resultPid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName.c_str()) == 0) {
                resultPid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return resultPid;
}


bool WindowOverlay::isRealWindow(HWND hwnd) {
    return (GetWindow(hwnd, GW_OWNER) == NULL) && IsWindowVisible(hwnd);
}


std::vector<HWND> WindowOverlay::getWindowsForPID(DWORD pid)
{
    std::vector<HWND> out;
    struct EnumData { DWORD pid; std::vector<HWND>* res; } data{ pid, &out };

    auto enumProc = [](HWND hwnd, LPARAM lparam)->BOOL {
        EnumData* d = reinterpret_cast<EnumData*>(lparam);
        if (!isRealWindow(hwnd)) return TRUE;

        DWORD wpid = 0;
        GetWindowThreadProcessId(hwnd, &wpid);
        if (wpid == d->pid) d->res->push_back(hwnd);
        return TRUE;
        };

    EnumWindows((WNDENUMPROC)enumProc, (LPARAM)&data);
    return out;
}


void WindowOverlay::positionOverlayOnTarget() {
    if (!g_overlayWnd || !g_targetWnd) return;

    RECT r;
    if (GetWindowRect(g_targetWnd, &r)) {
        SetWindowPos(g_overlayWnd, HWND_TOPMOST,
            r.left, r.top, r.right - r.left, r.bottom - r.top,
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}


#endif