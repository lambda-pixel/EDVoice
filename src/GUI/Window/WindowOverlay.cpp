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


DWORD GetProcessIdByName(const std::wstring& exeName)
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


bool IsRealWindow(HWND hwnd) {
    return (GetWindow(hwnd, GW_OWNER) == NULL) && IsWindowVisible(hwnd);
}


std::vector<HWND> GetWindowsForPID(DWORD pid)
{
    std::vector<HWND> out;
    struct EnumData { DWORD pid; std::vector<HWND>* res; } data{ pid, &out };

    auto enumProc = [](HWND hwnd, LPARAM lparam)->BOOL {
        EnumData* d = reinterpret_cast<EnumData*>(lparam);
        if (!IsRealWindow(hwnd)) return TRUE;

        DWORD wpid = 0;
        GetWindowThreadProcessId(hwnd, &wpid);
        if (wpid == d->pid) d->res->push_back(hwnd);
        return TRUE;
        };

    EnumWindows((WNDENUMPROC)enumProc, (LPARAM)&data);
    return out;
}


void PositionOverlayOnTarget() {
    if (!g_overlayWnd || !g_targetWnd) return;

    RECT r;
    if (GetWindowRect(g_targetWnd, &r)) {
        SetWindowPos(g_overlayWnd, HWND_TOPMOST,
            r.left, r.top, r.right - r.left, r.bottom - r.top,
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}


WindowOverlay::WindowOverlay(
    WindowSystem* sys,
    const std::string& title,
    const std::filesystem::path& config,
    const std::wstring& processName)
    : Window(sys, title, config)
{
    /*
    // Look for window for the given process name
    DWORD pid = GetProcessIdByName(processName);

    if (pid == 0) {
        throw std::runtime_error("Could not find process");
    }
    
    auto windows = GetWindowsForPID(pid);

    if (windows.empty()) {
        throw std::runtime_error("Could not find a window");
    }

    _targetWnd = windows[0];
    g_targetWnd = windows[0];

    _className = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(title);;

    WNDCLASSEXW wcx{};
    wcx.cbSize = sizeof(wcx);
    wcx.hInstance = _sys->_hInstance;
    wcx.lpfnWndProc = WindowOverlay::w32WndProc;
    wcx.lpszClassName = _className.c_str();
    //wcx.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    //wcx.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wcx.style = CS_VREDRAW | CS_HREDRAW;
    const ATOM result = ::RegisterClassExW(&wcx);

    if (!result) {
        throw std::runtime_error("failed to register window class");
    }

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
    DWORD exStyle2 = WS_POPUP;

    RECT targetRect;
    GetWindowRect(_targetWnd, &targetRect);
    int width = targetRect.right - targetRect.left;
    int height = targetRect.bottom - targetRect.top;

    _hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wcx.lpszClassName,
        L"Dear ImGui DirectX11 Example",
        WS_POPUP,
        targetRect.left, targetRect.top,
        width, height,
        nullptr, nullptr,
        wcx.hInstance,
        nullptr);

    g_overlayWnd = _hwnd;

    SetLayeredWindowAttributes(_hwnd, 0, 255, LWA_ALPHA);
    const MARGINS margin = { -1, 0, 0, 0 };
    DwmExtendFrameIntoClientArea(_hwnd, &margin);

    if (!_hwnd) {
        throw std::runtime_error("Could not create overlay");
    }

    ::ShowWindow(_hwnd, SW_SHOW);
    ::UpdateWindow(_hwnd);
    PositionOverlayOnTarget();

    HWINEVENTHOOK hook = SetWinEventHook(
        EVENT_OBJECT_CREATE, EVENT_OBJECT_LOCATIONCHANGE,
        NULL, w32WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    if (!hook) {
        throw std::runtime_error("SetWinEventHook failed");
    }
    */
}


WindowOverlay::~WindowOverlay()
{
#ifdef USE_SDL
    SDL_DestroyWindow(_sdlWindow);
#else
    DestroyWindow(_hwnd);
    UnregisterClassW(_className.c_str(), _sys->_hInstance);
#endif
}



void CALLBACK WindowOverlay::w32WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd,
    LONG, LONG, DWORD, DWORD)
{
    if (hwnd != g_targetWnd) return;

    if (event == EVENT_OBJECT_LOCATIONCHANGE || event == EVENT_OBJECT_CREATE) {
        PositionOverlayOnTarget();
    }
}

LRESULT WindowOverlay::w32WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    return ::DefWindowProcW(_hwnd, msg, wParam, lParam);
}


DWORD WindowOverlay::w32Style()
{
    return WS_POPUP;
}


DWORD WindowOverlay::dwExStyle()
{
    return WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
}

#endif