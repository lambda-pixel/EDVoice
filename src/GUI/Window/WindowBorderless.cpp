#include "WindowBorderless.h"

#ifdef USE_SDL
    #include <backends/imgui_impl_sdl3.h>
#else
    #include <windowsx.h>
    #include <dwmapi.h>
    #include <backends/imgui_impl_win32.h>
#endif


WindowBorderless::WindowBorderless(
    WindowSystem* sys,
    const std::string& title,
    const std::filesystem::path& config)
    : Window(sys, title, config)
{
#ifdef USE_SDL
    SDL_SetWindowHitTest(_sdlWindow, sdlHitTest, this);
    SDL_SetWindowBordered(_sdlWindow, false);
    SDL_ShowWindow(_sdlWindow);
#else
    ::SetWindowLongPtrW(_hwnd, GWL_STYLE, static_cast<LONG>(w32Style()));

    if (w32CompositionEnabled()) {
        static const MARGINS shadow_state[2]{ { 0,0,0,0 },{ 1,1,1,1 } };
        ::DwmExtendFrameIntoClientArea(_hwnd, &shadow_state[_borderless ? 1 : 0]);
    }

    ::SetWindowPos(_hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
    ::ShowWindow(_hwnd, SW_SHOW);
    ::UpdateWindow(_hwnd);
#endif
    // Apply scale
    _titlebarHeight = _mainScale * 32.f;
    _buttonWidth = _mainScale * 55.f;
    _totalButtonWidth = 3.f * _buttonWidth;
}


WindowBorderless::~WindowBorderless()
{}


void WindowBorderless::minimizeWindow()
{
#ifdef USE_SDL
    SDL_MinimizeWindow(_sdlWindow);
#else
    PostMessage(_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
#endif
}


void WindowBorderless::maximizeRestoreWindow()
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


void WindowBorderless::closeWindow()
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


void WindowBorderless::openVoicePackFileDialog(void* userdata, openedFile callback)
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

LRESULT WindowBorderless::w32WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_NCCALCSIZE: {
            if (wParam == TRUE && _borderless) {
                auto& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                w32AdjustMaximizedClientRect(params.rgrc[0]);
                return 0;
            }
            break;
        }

        case WM_NCHITTEST: {
            // When we have no border or title bar, we need to perform our
            // own hit testing to allow resizing and moving.
            if (_borderless) {
                LRESULT hitResult = w32HitTest(
                    POINT{
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
            if (!w32CompositionEnabled()) {
                // Prevents window frame reappearing on window activation
                // in "basic" theme, where no aero shadow is present.
                return 1;
            }
            break;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            switch (wParam) {
                case VK_F10: { w32SetBorderless(!_borderless);               return 0; }
            }
            break;
        }
    }

    return ::DefWindowProcW(_hwnd, msg, wParam, lParam);
}



DWORD WindowBorderless::w32Style()
{
    if (_borderless) {
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

bool WindowBorderless::w32CompositionEnabled()
{
    BOOL compositionEnabled = false;
    const HRESULT queryComposition = ::DwmIsCompositionEnabled(&compositionEnabled);

    return compositionEnabled && (queryComposition == S_OK);
}

void WindowBorderless::w32SetBorderless(bool borderless)
{
    if (borderless != _borderless) {
        _borderless = borderless;

        ::SetWindowLongPtrW(_hwnd, GWL_STYLE, static_cast<LONG>(w32Style()));

        if (w32CompositionEnabled()) {
            static const MARGINS shadow_state[2]{ { 0,0,0,0 },{ 1,1,1,1 } };
            ::DwmExtendFrameIntoClientArea(_hwnd, &shadow_state[_borderless ? 1 : 0]);
        }

        ::SetWindowPos(_hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
        ::ShowWindow(_hwnd, SW_SHOW);
        ::UpdateWindow(_hwnd);
    }
}


bool WindowBorderless::w32IsMaximized()
{
    WINDOWPLACEMENT placement = {};

    if (!::GetWindowPlacement(_hwnd, &placement)) {
        return false;
    }

    return placement.showCmd == SW_MAXIMIZE;
}


void WindowBorderless::w32AdjustMaximizedClientRect(RECT& rect)
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


LRESULT WindowBorderless::w32HitTest(POINT cursor) const
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


std::string WindowBorderless::w32OpenFileName(const char* title, const char* initialDir, const char* filter, bool multiSelect)
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
