#include "WindowMain.h"

#ifdef USE_SDL
    #include <backends/imgui_impl_sdl3.h>
#else
    #include <windowsx.h>
    #include <dwmapi.h>
    #include <commdlg.h>
    #include <codecvt>

    #include <backends/imgui_impl_win32.h>
#endif

#include <backends/imgui_impl_vulkan.h>

#include "inter.cpp"


WindowMain::WindowMain(
    WindowSystem* sys,
    const std::string& title,
    const std::filesystem::path& config)
    : Window(sys, title, config)
{
#ifdef USE_SDL
    SDL_WindowFlags window_flags =
        SDL_WINDOW_VULKAN |
        SDL_WINDOW_RESIZABLE |
        // SDL_WINDOW_HIDDEN |
        SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_BORDERLESS;

    _sdlWindow = SDL_CreateWindow(
        title.c_str(),
        640, 700,
        window_flags
    );

    _mainScale = SDL_GetWindowPixelDensity(_sdlWindow);
    _vkAdapter.initDevice(this);

    SDL_SetWindowPosition(_sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetWindowHitTest(_sdlWindow, sdlHitTest, this);
    SDL_ShowWindow(_sdlWindow);

    IMGUI_CHECKVERSION();
    _imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(_imGuiContext);

    ImGui_ImplSDL3_InitForVulkan(_sdlWindow);
#else
    ImGui_ImplWin32_EnableDpiAwareness();
    _mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    _className = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(title);;

    WNDCLASSEXW wcx{};
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.hInstance = _sys->_hInstance;
    wcx.lpfnWndProc = WindowMain::w32WndProc;
    wcx.lpszClassName = _className.c_str();
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcx.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    const ATOM result = ::RegisterClassExW(&wcx);

    if (!result) {
        throw std::runtime_error("failed to register window class");
    }

    _hwnd = ::CreateWindowExW(
        0,
        wcx.lpszClassName,
        _className.c_str(),
        w32Style(),
        CW_USEDEFAULT, CW_USEDEFAULT,
        (int)(_mainScale * 640), (int)(_mainScale * 700),
        nullptr,
        nullptr,
        nullptr,
        this
    );

    if (w32CompositionEnabled()) {
        static const MARGINS shadow_state[2]{ { 0,0,0,0 },{ 1,1,1,1 } };
        ::DwmExtendFrameIntoClientArea(_hwnd, &shadow_state[_borderlessWindow ? 1 : 0]);
    }

    // Center window to the screen
    RECT rc;
    GetWindowRect(_hwnd, &rc);
    int winWidth = rc.right - rc.left;
    int winHeight = rc.bottom - rc.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int x = (screenWidth - winWidth) / 2;
    int y = (screenHeight - winHeight) / 2;

    ::SetWindowPos(_hwnd, nullptr, x, y, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE);
    ::ShowWindow(_hwnd, _sys->_nShowCmd);
    ::UpdateWindow(_hwnd);

    _vkAdapter.initDevice(this);

    IMGUI_CHECKVERSION();
    _imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(_imGuiContext);

    ImGui_ImplWin32_Init(_hwnd);
#endif

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

    refreshResize();

    // Apply scale
    _titlebarHeight = _mainScale * 32.f;
    _buttonWidth = _mainScale * 55.f;
    _totalButtonWidth = 3.f * _buttonWidth;
}


WindowMain::~WindowMain()
{
    vkDeviceWaitIdle(_vkAdapter.getDevice());

    ImGui::SetCurrentContext(_imGuiContext);
    ImGui::SaveIniSettingsToDisk(_configPath.string().c_str());

    ImGui_ImplVulkan_Shutdown();

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


void WindowMain::minimizeWindow()
{
#ifdef USE_SDL
    SDL_MinimizeWindow(_sdlWindow);
#else
    PostMessage(_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
#endif
}


void WindowMain::maximizeRestoreWindow()
{
#ifdef USE_SDL
    if (_isMaximized) {
        SDL_RestoreWindow(_sdlWindow);
    }
    else {
        SDL_MaximizeWindow(_sdlWindow);
    }

    _isMaximized = !_isMaximized;
#else
    PostMessage(_hwnd, WM_SYSCOMMAND, IsZoomed(_hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
#endif
}


void WindowMain::closeWindow()
{
#ifdef USE_SDL
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    event.window.windowID = SDL_GetWindowID(_sdlWindow);
    SDL_PushEvent(&event);
#else
    PostMessage(_hwnd, WM_CLOSE, 0, 0);
#endif
}


void WindowMain::openVoicePackFileDialog(void* userdata, openedFile callback)
{
#ifdef USE_SDL
    const SDL_DialogFileFilter filters[] = {
    { "JSON file",  "json" }
    };

    // TODO: Ugly but whatever... it is deleted by the callback
    OpenFileCbData* callbackData = new OpenFileCbData;
    callbackData->callback = callback;
    callbackData->userdata = userdata;

    SDL_ShowOpenFileDialog(
        sdlCallbackOpenFile,
        callbackData,
        _sdlWindow,
        filters, 1,
        NULL,
        false);
#else
    const std::string newVoicePack = w32OpenFileName(
        "Select voicepack file",
        "",
        "JSON file\0*.json\0",
        false);

    callback(userdata, newVoicePack);
#endif
}


// ----------------------------------------------------------------------------
// Platform specific mess
// ----------------------------------------------------------------------------


#ifdef USE_SDL

void WindowMain::sdlWndProc(SDL_Event& event)
{
    ImGui_ImplSDL3_ProcessEvent(&event);

    switch (event.type) {
        case SDL_EVENT_QUIT:
            _closed = true;
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (event.window.windowID == SDL_GetWindowID(_sdlWindow)) {
                _closed = true;
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
        {
            int width, height;
            SDL_GetWindowSizeInPixels(_sdlWindow, &width, &height);
            onResize(width, height);
        }
        break;

        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        {
            _mainScale = SDL_GetWindowDisplayScale(_sdlWindow);
            int width, height;
            SDL_GetWindowSizeInPixels(_sdlWindow, &width, &height);
            onResize(width, height);
        }
        break;
        default:
            break;
    }
}


SDL_HitTestResult SDLCALL WindowMain::sdlHitTest(SDL_Window* win, const SDL_Point* area, void* data)
{
    WindowMain* obj = (WindowMain*)data;
    assert(win == obj->_sdlWindow);

    int width, height;
    SDL_GetWindowSize(obj->_sdlWindow, &width, &height);

    const int borderX = 8; // Horizontal resize border thickness in pixels
    const int borderY = 8; // Vertical resize border thickness in pixels

    int x = area->x;
    int y = area->y;

    enum RegionMask {
        client = 0b0000,
        left = 0b0001,
        right = 0b0010,
        top = 0b0100,
        bottom = 0b1000,
    };

    int result =
        ((x < borderX) ? left : 0) |
        ((x >= (width - borderX)) ? right : 0) |
        ((y < borderY) ? top : 0) |
        ((y >= (height - borderY)) ? bottom : 0);

    switch (result) {
        case left:              return SDL_HITTEST_RESIZE_LEFT;
        case right:             return SDL_HITTEST_RESIZE_RIGHT;
        case top:               return SDL_HITTEST_RESIZE_TOP;
        case bottom:            return SDL_HITTEST_RESIZE_BOTTOM;
        case top | left:        return SDL_HITTEST_RESIZE_TOPLEFT;
        case top | right:       return SDL_HITTEST_RESIZE_TOPRIGHT;
        case bottom | left:     return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        case bottom | right:    return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        case client: {
            // Title bar area � allow dragging the window
            if (y < obj->_titlebarHeight && x < (width - obj->_totalButtonWidth)) {
                return SDL_HITTEST_DRAGGABLE;
            }
            else {
                return SDL_HITTEST_NORMAL;
            }
        }
        default:                return SDL_HITTEST_NORMAL;
    }
}


void SDLCALL WindowMain::sdlCallbackOpenFile(void* userdata, const char* const* filelist, int filter)
{
    OpenFileCbData* obj = (OpenFileCbData*)userdata;

    if (!filelist) {
        SDL_Log("An error occured: %s", SDL_GetError());
    }
    else if (!*filelist) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
    }
    else {
        while (*filelist) {
            const std::string newVoicePack = *filelist;

            obj->callback(obj->userdata, newVoicePack);

            filelist++;
        }
    }

    delete obj;
}


#else


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowMain::w32WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto pWindow = reinterpret_cast<WindowMain*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if (!pWindow) {
        if (msg == WM_NCCREATE) {
            auto userdata = reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams;
            // store window instance pointer in window user data
            ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userdata));
        }

        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    else if (pWindow->_hwnd == hWnd) {
        if (pWindow->_imGuiInitialized) {
            ImGuiContext* prevContex = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(pWindow->_imGuiContext);
            LRESULT imGuiRes = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
            ImGui::SetCurrentContext(prevContex);

            if (imGuiRes) {
                return 0;
            }
        }

        switch (msg) {
            case WM_SIZE: {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                pWindow->onResize(width, height);
                break;
            }

            case WM_NCCALCSIZE: {
                if (wParam == TRUE && pWindow->_borderlessWindow) {
                    auto& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                    pWindow->w32AdjustMaximizedClientRect(params.rgrc[0]);
                    return 0;
                }
                break;
            }
            case WM_NCHITTEST: {
                // When we have no border or title bar, we need to perform our
                // own hit testing to allow resizing and moving.
                if (pWindow->_borderlessWindow) {
                    LRESULT hitResult = pWindow->w32HitTest(POINT{
                        GET_X_LPARAM(lParam),
                        GET_Y_LPARAM(lParam)
                        });

                    if (hitResult) {
                        return hitResult;
                    }
                }
                break;
            }
            case WM_NCACTIVATE: {
                if (!pWindow->w32CompositionEnabled()) {
                    // Prevents window frame reappearing on window activation
                    // in "basic" theme, where no aero shadow is present.
                    return 1;
                }
                break;
            }

            case WM_CLOSE: {
                ::DestroyWindow(hWnd);
                pWindow->_closed = true;
                return 0;
            }

            case WM_DESTROY: {
                ::PostQuitMessage(0);
                pWindow->_closed = true;
                return 0;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN: {
                switch (wParam) {
                    //case VK_F8: { window.borderless_drag = !window.borderless_drag;        return 0; }
                    //case VK_F9: { window.borderless_resize = !window.borderless_resize;    return 0; }
                    //case VK_F10: { window.set_borderless(!window._borderlessWindow);               return 0; }
                    case VK_F10: { pWindow->w32SetBorderless(!pWindow->_borderlessWindow);               return 0; }
                    //case VK_F11: { window.set_borderless_shadow(!window.borderless_shadow); return 0; }
                }
                break;
            }
        }
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


bool WindowMain::w32CompositionEnabled()
{
    BOOL compositionEnabled = false;
    const HRESULT queryComposition = ::DwmIsCompositionEnabled(&compositionEnabled);

    return compositionEnabled && (queryComposition == S_OK);
}


DWORD WindowMain::w32Style()
{
    if (_borderlessWindow) {
        if (w32CompositionEnabled()) {
            return WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
        }
        else {
            return WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
        }
    }
    else {
        return WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    }
}


void WindowMain::w32SetBorderless(bool borderless)
{
    if (borderless != _borderlessWindow) {
        _borderlessWindow = borderless;

        ::SetWindowLongPtrW(_hwnd, GWL_STYLE, static_cast<LONG>(w32Style()));

        if (w32CompositionEnabled()) {
            static const MARGINS shadow_state[2]{ { 0,0,0,0 },{ 1,1,1,1 } };
            ::DwmExtendFrameIntoClientArea(_hwnd, &shadow_state[_borderlessWindow ? 1 : 0]);
        }

        ::SetWindowPos(_hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
        ::ShowWindow(_hwnd, SW_SHOW);
        ::UpdateWindow(_hwnd);
    }
}


bool WindowMain::w32IsMaximized()
{
    WINDOWPLACEMENT placement = {};

    if (!::GetWindowPlacement(_hwnd, &placement)) {
        return false;
    }

    return placement.showCmd == SW_MAXIMIZE;
}


void WindowMain::w32AdjustMaximizedClientRect(RECT& rect)
{
    if (!w32IsMaximized()) {
        return;
    }

    auto monitor = ::MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONULL);
    if (!monitor) {
        return;
    }

    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!::GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }

    // when maximized, make the client area fill just the monitor (without task bar) rect,
    // not the whole window rect which extends beyond the monitor.
    rect = monitor_info.rcWork;
}


LRESULT WindowMain::w32HitTest(POINT cursor) const
{
    // identify borders and corners to allow resizing the window.
    // Note: On Windows 10, windows behave differently and
    // allow resizing outside the visible window frame.
    // This implementation does not replicate that behavior.
    const POINT border{
        ::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
        ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
    };
    RECT window;
    if (!::GetWindowRect(_hwnd, &window)) {
        return HTNOWHERE;
    }

    enum region_mask {
        client = 0b0000,
        left = 0b0001,
        right = 0b0010,
        top = 0b0100,
        bottom = 0b1000,
    };

    const auto result =
        left * (cursor.x < (window.left + border.x)) |
        right * (cursor.x >= (window.right - border.x)) |
        top * (cursor.y < (window.top + border.y)) |
        bottom * (cursor.y >= (window.bottom - border.y));

    switch (result) {
        case left: return HTLEFT;
        case right: return HTRIGHT;
        case top: return HTTOP;
        case bottom: return HTBOTTOM;
        case top | left: return HTTOPLEFT;
        case top | right: return HTTOPRIGHT;
        case bottom | left: return HTBOTTOMLEFT;
        case bottom | right: return HTBOTTOMRIGHT;
        case client: {
            // TODO: Adjust
            if (cursor.y < (window.top + _titlebarHeight) && cursor.x < (window.right - _totalButtonWidth)) {
                return HTCAPTION;
            }
            else {
                return HTCLIENT;
            }
        }
        default: return HTNOWHERE;
    }
}


std::string WindowMain::w32OpenFileName(const char* title, const char* initialDir, const char* filter, bool multiSelect)
{
    OPENFILENAMEA ofn = { 0 };
    char fileBuffer[MAX_PATH * 4] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = _hwnd;
    ofn.lpstrTitle = title;
    ofn.lpstrInitialDir = initialDir;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = sizeof(fileBuffer);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (multiSelect) {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    }

    if (GetOpenFileNameA(&ofn)) {
        return std::string(fileBuffer);
    }
    return {};
}

#endif
