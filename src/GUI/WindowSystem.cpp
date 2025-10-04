#include "WindowSystem.h"

#include <stdexcept>

#ifdef USE_SDL
    #include <backends/imgui_impl_sdl3.h>
#else
    const wchar_t CLASS_NAME[] = L"EDVoice";

    #include <windowsx.h>
    #include <dwmapi.h>
    #include <commdlg.h>

    #include <backends/imgui_impl_win32.h>
#endif

#include <backends/imgui_impl_vulkan.h>


#ifdef BUILD_MEDICORP
    const wchar_t WINDOW_TITLE[] = L"EDVoice - MediCorp Edition";
    const char WINDOW_TITLE_STD[] = "EDVoice - MediCorp Edition";
#else
    const wchar_t WINDOW_TITLE[] = L"EDVoice";
    const char WINDOW_TITLE_STD[] = "EDVoice";
#endif


WindowSystem::WindowSystem(
#ifdef USE_SDL
)
#else
    HINSTANCE hInstance, int nShowCmd)
    : _hInstance(hInstance)
    , _nShowCmd(nShowCmd)
#endif
{
#if defined(USE_SDL) || defined(USE_SDL_MIXER)
    SDL_InitFlags sdlFlags = 0;
#endif

#ifdef USE_SDL
    sdlFlags |= SDL_INIT_VIDEO | SDL_INIT_GAMEPAD;
#endif

#ifdef USE_SDL_MIXER
    sdlFlags |= SDL_INIT_AUDIO;
 #endif

#if defined(USE_SDL) || defined(USE_SDL_MIXER)
    SDL_Init(sdlFlags);
#endif
}


WindowSystem::~WindowSystem()
{
#if defined(USE_SDL) || defined(USE_SDL_MIXER)
    SDL_Quit();
#endif
}


void WindowSystem::createWindow(VkAdapter* vkAdapter)
{
    _vkAdapter = vkAdapter;

#ifdef USE_SDL
    SDL_WindowFlags window_flags =
        SDL_WINDOW_VULKAN |
        SDL_WINDOW_RESIZABLE |
        // SDL_WINDOW_HIDDEN |
        SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_BORDERLESS;

    _sdlWindow = SDL_CreateWindow(
        "EDVoice",
        640, 700,
        window_flags
    );

    _mainScale = SDL_GetWindowPixelDensity(_sdlWindow);
    _vkAdapter->initDevice();

    SDL_SetWindowPosition(_sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetWindowHitTest(_sdlWindow, sdlHitTest, this);
    SDL_ShowWindow(_sdlWindow);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplSDL3_InitForVulkan(_sdlWindow);
#else
    ImGui_ImplWin32_EnableDpiAwareness();
    _mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    const wchar_t CLASS_NAME[] = L"EDVoice";

    WNDCLASSEXW wcx{};
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.hInstance = nullptr;
    wcx.lpfnWndProc = WindowSystem::w32WndProc;
    wcx.lpszClassName = CLASS_NAME;
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcx.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    const ATOM result = ::RegisterClassExW(&wcx);

    if (!result) {
        throw std::runtime_error("failed to register window class");
    }

    _hwnd = ::CreateWindowExW(
        0,
        wcx.lpszClassName,
        WINDOW_TITLE,
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
    ::ShowWindow(_hwnd, _nShowCmd);
    ::UpdateWindow(_hwnd);

    _vkAdapter->initDevice();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplWin32_Init(_hwnd);
#endif
}


void WindowSystem::render()
{
#ifdef USE_SDL
    const int TARGET_FPS = 60;
    const int FRAME_DELAY_MS = 1000 / TARGET_FPS;

    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 last = 0;
    double deltaTime = 0;

    if (deltaTime < FRAME_DELAY_MS) {
        SDL_Delay((Uint32)(FRAME_DELAY_MS - deltaTime));
    }

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        sdlWndProc(event);
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
#else
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
#endif
}


void WindowSystem::destroyWindow()
{
    ImGui_ImplVulkan_Shutdown();

#ifdef USE_SDL
    ImGui_ImplSDL3_Shutdown();
    SDL_DestroyWindow(_sdlWindow);
#else
    ImGui_ImplWin32_Shutdown();

    DestroyWindow(_hwnd);
    UnregisterClassW(CLASS_NAME, _hInstance);
#endif
}



void WindowSystem::minimizeWindow()
{
#ifdef USE_SDL
    SDL_MinimizeWindow(_sdlWindow);
#else
    PostMessage(_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
#endif
}


void WindowSystem::maximizeRestoreWindow()
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


void WindowSystem::closeWindow()
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


const char* WindowSystem::windowTitle() const
{
    return WINDOW_TITLE_STD;
}


void WindowSystem::getVkInstanceExtensions(std::vector<const char*>& extensions) const
{
#ifdef USE_SDL
    uint32_t nInstanceExt;
    const char* const* instanceExt = SDL_Vulkan_GetInstanceExtensions(&nInstanceExt);

    for (size_t i = 0; i < nInstanceExt; i++) {
        extensions.push_back(instanceExt[i]);
    }
#else
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back("VK_KHR_win32_surface");
#endif
}


void WindowSystem::createVkSurfaceKHR(
    VkInstance instance,
    VkSurfaceKHR* surface, int* width, int* height) const
{
#ifdef USE_SDL
    SDL_Vulkan_CreateSurface(_sdlWindow, instance, nullptr, surface);

    // WTF... I have to call GetWindowSize first and then GetWindowSizeInPixel just for
    // lord Wayland. Damn
    SDL_GetWindowSize(_sdlWindow, width, height);
#else
    // Create a Vulkan surface (WIN32 specific)
    const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, // sType, pNext
        0,                                                        // flags
        _hInstance,                                                // hinstance
        _hwnd                                                      // hwnd
    };

    vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface);

    RECT rect;
    ::GetClientRect(_hwnd, &rect);
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
#endif
}


// ----------------------------------------------------------------------------
// Platform specific mess
// ----------------------------------------------------------------------------

#ifdef USE_SDL

void WindowSystem::sdlWndProc(SDL_Event& event)
{
    ImGui_ImplSDL3_ProcessEvent(&event);

    switch (event.type) {
    case SDL_EVENT_QUIT:
        _quit = true;
        break;

    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (event.window.windowID == SDL_GetWindowID(_sdlWindow)) {
            _quit = true;
        }
        break;

    case SDL_EVENT_WINDOW_RESIZED:
    {
        int width, height;
        SDL_GetWindowSizeInPixels(_sdlWindow, &width, &height);
        _vkAdapter->resized(width, height);
    }
    break;

    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
    {
        _mainScale = SDL_GetWindowDisplayScale(_sdlWindow);
        int width, height;
        SDL_GetWindowSizeInPixels(_sdlWindow, &width, &height);
        _vkAdapter->resized(width, height);
    }
    break;
    default:
        break;
    }
}


SDL_HitTestResult SDLCALL WindowSystem::sdlHitTest(SDL_Window* win, const SDL_Point* area, void* data)
{
    WindowSystem* obj = (WindowSystem*)data;
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
        // Title bar area — allow dragging the window
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


void SDLCALL WindowSystem::sdlCallbackOpenFile(void* userdata, const char* const* filelist, int filter)
{
    // TODO!
    //WindowSystem* obj = (WindowSystem*)userdata;
    //VoicePackManager& voicepack = obj->_app->getVoicepack();

    //if (!filelist) {
    //    obj->_logErrStr = SDL_GetError();
    //    obj->_hasError = true;
    //    SDL_Log("An error occured: %s", SDL_GetError());
    //    return;
    //}
    //else if (!*filelist) {
    //    SDL_Log("The user did not select any file.");
    //    SDL_Log("Most likely, the dialog was canceled.");
    //    return;
    //}

    //while (*filelist) {
    //    const std::string newVoicePack = *filelist;
    //    const std::string voicepackName = std::filesystem::path(newVoicePack).stem().string();
    //    const size_t idxNewVoicePack = voicepack.addVoicePack(voicepackName, newVoicePack);
    //    voicepack.loadVoicePackByIndex(idxNewVoicePack);

    //    SDL_Log("Full path to selected file: '%s'", *filelist);
    //    filelist++;
    //}
}

#else

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowSystem::w32WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    if (msg == WM_NCCREATE) {
        auto userdata = reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams;
        // store window instance pointer in window user data
        ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userdata));
    }

    if (auto window_ptr = reinterpret_cast<WindowSystem*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA))) {
        auto& window = *window_ptr;

        switch (msg) {
            case WM_SIZE: {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                window._vkAdapter->resized(width, height);
            break;
        }

        case WM_NCCALCSIZE: {
            if (wParam == TRUE && window._borderlessWindow) {
                auto& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                window.w32AdjustMaximizedClientRect(params.rgrc[0]);
                return 0;
            }
            break;
        }
        case WM_NCHITTEST: {
            // When we have no border or title bar, we need to perform our
            // own hit testing to allow resizing and moving.
            if (window._borderlessWindow) {
                LRESULT hitResult = window.w32HitTest(POINT{
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
            if (!window.w32CompositionEnabled()) {
                // Prevents window frame reappearing on window activation
                // in "basic" theme, where no aero shadow is present.
                return 1;
            }
            break;
        }

        case WM_CLOSE: {
            ::DestroyWindow(hWnd);
            window._quit = true;
            return 0;
        }

        case WM_DESTROY: {
            ::PostQuitMessage(0);
            window._quit = true;
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            switch (wParam) {
                //case VK_F8: { window.borderless_drag = !window.borderless_drag;        return 0; }
                //case VK_F9: { window.borderless_resize = !window.borderless_resize;    return 0; }
                //case VK_F10: { window.set_borderless(!window._borderlessWindow);               return 0; }
            case VK_F10: { window.w32SetBorderless(!window._borderlessWindow);               return 0; }
                       //case VK_F11: { window.set_borderless_shadow(!window.borderless_shadow); return 0; }
            }
            break;
        }
        }
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


bool WindowSystem::w32CompositionEnabled()
{
    BOOL compositionEnabled = false;
    const HRESULT queryComposition = ::DwmIsCompositionEnabled(&compositionEnabled);

    return compositionEnabled && (queryComposition == S_OK);
}


DWORD WindowSystem::w32Style()
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


void WindowSystem::w32CreateWindow(int nShowCmd)
{
    WNDCLASSEXW wcx{};
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.hInstance = nullptr;
    wcx.lpfnWndProc = WindowSystem::w32WndProc;
    wcx.lpszClassName = CLASS_NAME;
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcx.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    const ATOM result = ::RegisterClassExW(&wcx);

    if (!result) {
        throw std::runtime_error("failed to register window class");
    }

    _hwnd = ::CreateWindowExW(
        0,
        wcx.lpszClassName,
        WINDOW_TITLE,
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
    ::ShowWindow(_hwnd, nShowCmd);
    ::UpdateWindow(_hwnd);
}


void WindowSystem::w32SetBorderless(bool borderless)
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


bool WindowSystem::w32IsMaximized()
{
    WINDOWPLACEMENT placement = {};
    if (!::GetWindowPlacement(_hwnd, &placement)) {
        return false;
    }

    return placement.showCmd == SW_MAXIMIZE;
}


void WindowSystem::w32AdjustMaximizedClientRect(RECT& rect)
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


LRESULT WindowSystem::w32HitTest(POINT cursor) const
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

    const auto drag = false;//borderless_drag ? HTCAPTION : HTCLIENT;

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


std::string WindowSystem::w32OpenFileName(const char* title, const char* initialDir, const char* filter, bool multiSelect)
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
    if (multiSelect)
        ofn.Flags |= OFN_ALLOWMULTISELECT;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(fileBuffer);
    }
    return {};
}

#endif


