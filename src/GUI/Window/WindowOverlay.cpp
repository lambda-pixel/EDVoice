#include "WindowOverlay.h"

#ifndef USE_SDL

#include <windowsx.h>
#include <dwmapi.h>
#include <commdlg.h>
#include <codecvt>
#include <tlhelp32.h>

#include <backends/imgui_impl_win32.h>
//#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_dx11.h>

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

    ImGui_ImplWin32_EnableDpiAwareness();
    _mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

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


    _d3dAdapter.initDevice(this);
    //_vkAdapter.initDevice(this);

    IMGUI_CHECKVERSION();
    _imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(_imGuiContext);

    // Final ImGui setup
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    ImGui::LoadIniSettingsFromDisk(_configPath.string().c_str());

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(getMainScale());
    style.FontScaleDpi = getMainScale();

    ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(
        inter_compressed_data,
        inter_compressed_size,
        getMainScale() * 20.f);

    ImGui_ImplWin32_Init(_hwnd);

    //refreshResize();

    ImGui_ImplDX11_Init(_d3dAdapter.device(), _d3dAdapter.deviceContext());
}


WindowOverlay::~WindowOverlay()
{
    vkDeviceWaitIdle(_vkAdapter.getDevice());

    ImGui::SetCurrentContext(_imGuiContext);
    ImGui::SaveIniSettingsToDisk(_configPath.string().c_str());

    ImGui_ImplDX11_Shutdown();
    //ImGui_ImplVulkan_Shutdown();

#ifdef USE_SDL
    ImGui_ImplSDL3_Shutdown();
    SDL_DestroyWindow(_sdlWindow);
#else
    ImGui_ImplWin32_Shutdown();

    DestroyWindow(_hwnd);
    UnregisterClassW(_className.c_str(), _sys->_hInstance);
#endif

    ImGui::DestroyContext(_imGuiContext);
}


void WindowOverlay::beginFrame()
{
    ImGui::SetCurrentContext(_imGuiContext);

    MSG msg;
    while (PeekMessage(&msg, _hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();
    _d3dAdapter.startNewFrame();
}


void WindowOverlay::endFrame()
{
    // No ImGui context swicthing shall happen there,
    // no other context is supposed to happen in between
    // beginFrame() & endFrame()

    assert(_imGuiContext == ImGui::GetCurrentContext());

    // Rendering
    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    _d3dAdapter.renderFrame();
    _d3dAdapter.presentFrame();
}


void WindowOverlay::onResize(uint32_t width, uint32_t height)
{
    _d3dAdapter.resized(width, height);

    //if (_imGuiInitialized) {
    //    refreshResize();
    //}
}


void CALLBACK WindowOverlay::w32WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd,
    LONG, LONG, DWORD, DWORD)
{
    if (hwnd != g_targetWnd) return;

    if (event == EVENT_OBJECT_LOCATIONCHANGE || event == EVENT_OBJECT_CREATE) {
        PositionOverlayOnTarget();
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowOverlay::w32WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto pWindow = reinterpret_cast<WindowOverlay*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if (!pWindow) {
        if (msg == WM_NCCREATE) {
            auto userdata = reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams;
            // store window instance pointer in window user data
            ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userdata));
        }

        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    else if (pWindow->_hwnd == hWnd) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
            case WM_SIZE:
                if (wParam == SIZE_MINIMIZED)
                    return 0;
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                pWindow->onResize(width, height);
                return 0;
        }
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}



#endif