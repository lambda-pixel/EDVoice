#include "EDVoiceGUI.h"

#include <stdexcept>

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <backends/imgui_impl_vulkan.h>


const wchar_t CLASS_NAME[] = L"EDVoice";
const wchar_t WINDOW_TITLE[] = L"EDVoice";

#include "roboto.cpp"

EDVoiceGUI::EDVoiceGUI(HINSTANCE hInstance, int nShowCmd)
    : _vkAdapter({ VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface" })
{
    // ImGui initialization
    ImGui_ImplWin32_EnableDpiAwareness();
    _mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Win32 stuff
    w32CreateWindow(nShowCmd);
    _vkAdapter.initDevice(hInstance, _hwnd);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    ImGui::StyleColorsDark();

    // Scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(_mainScale);
    style.FontScaleDpi = _mainScale;

    ImGui_ImplWin32_Init(_hwnd);

    ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(Roboto_compressed_data, Roboto_compressed_size, _mainScale * 20.f);

    resize();
}


EDVoiceGUI::~EDVoiceGUI()
{
    vkDeviceWaitIdle(_vkAdapter.getDevice());
    
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();

    DestroyWindow(_hwnd);
    UnregisterClassW(CLASS_NAME, _hInstance);
}


void EDVoiceGUI::beginMainWindow()
{
    ImGuiViewport* pViewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowSize(pViewport->Size);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("Main", nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDecoration);

    if (_borderlessWindow) {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float titleMarginLeft = 8.f;
        const float buttonWidth = 55.f;
        const ImVec2 buttonSize(buttonWidth, _titlebarHeight);

        _totalButtonWidth = 3 * buttonWidth;

        // Center title vertically
        ImGui::PushFont(NULL, style.FontSizeBase * 1.2f);
        ImGui::SetCursorPos(ImVec2(titleMarginLeft, .5f * (_titlebarHeight - ImGui::GetFontSize())));
        ImGui::Text("EDVoice");
        ImGui::PopFont();

        // Minimize & Resize buttons
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 20));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 50));
        {
            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - 3 * buttonWidth, 0.));
            if (ImGui::Button("-", buttonSize)) { PostMessage(_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0); }

            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - 2 * buttonWidth, 0.));
            if (ImGui::Button("+", buttonSize)) { PostMessage(_hwnd, WM_SYSCOMMAND, IsZoomed(_hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0); }
        }
        ImGui::PopStyleColor(3);

        // Close button
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 125));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 50, 50, 200));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 0, 0, 255));
        {
            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - buttonWidth, 0.));
            if (ImGui::Button("x", buttonSize)) { PostMessage(_hwnd, WM_CLOSE, 0, 0); }
        }
        ImGui::PopStyleColor(3);

        // Title separator
        ImU32 col = ImGui::GetColorU32(ImGuiCol_Separator);

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(0, _titlebarHeight),
            ImVec2(pViewport->Size.x, _titlebarHeight),
            col, 1.0f
        );
    }
 }


void EDVoiceGUI::endMainWindow()
{
    ImGui::End();
}


void EDVoiceGUI::run()
{
    bool show_demo_window = true;
    bool quit = false;

    while (!quit)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                quit = true;
            }
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        beginMainWindow();

        ImGui::Text("Hello World!");

        endMainWindow();

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
            
        if (!is_minimized)
        {
            VkCommandBuffer commandBuffer = _vkAdapter.startNewFrame();

            if (commandBuffer != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
                _vkAdapter.renderFrame();
                _vkAdapter.presentFrame();
            }
            else {
                resize();
            }
        }
    }
}


void EDVoiceGUI::resize()
{
    if (_imGuiInitialized) {
        ImGui_ImplVulkan_Shutdown();
    }

    ImGui_ImplVulkan_InitInfo init_info = {
        _vkAdapter.API_VERSION,             // ApiVersion
        _vkAdapter.getInstance(),           // Instance
        _vkAdapter.getPhysicalDevice(),     // PhysicalDevice
        _vkAdapter.getDevice(),             // Device
        _vkAdapter.getQueueFamily(),        // QueueFamily
        _vkAdapter.getQueue(),              // Queue
        VK_NULL_HANDLE,                     // DescriptorPool
        IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE, // DescriptorPoolSize
        _vkAdapter.nImageCount(),           // MinImageCount
        _vkAdapter.nImageCount(),           // ImageCount
        VK_NULL_HANDLE,                     // PipelineCache (optional)
        _vkAdapter.getRenderPass(),         // RenderPass
        0,                                  // Subpass
        VK_SAMPLE_COUNT_1_BIT,              // msaaSamples
        false,                              // UseDynamicRendering
    #ifdef IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
        {},                                 // PipelineRenderingCreateInfo (optional)
    #endif
        nullptr,                            // VkAllocationCallbacks
        nullptr,                            // (*CheckVkResultFn)(VkResult err)
        1024 * 1024                           // MinAllocationSize
    };

    ImGui_ImplVulkan_Init(&init_info);

    _imGuiInitialized = true;
}


// ----------------------------------------------------------------------------
// WIN32 specific mess
// ----------------------------------------------------------------------------


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK EDVoiceGUI::w32WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    if (msg == WM_NCCREATE) {
        auto userdata = reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams;
        // store window instance pointer in window user data
        ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userdata));
    }
    if (auto window_ptr = reinterpret_cast<EDVoiceGUI*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA))) {
        auto& window = *window_ptr;

        switch (msg) {
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
            return 0;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
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


bool EDVoiceGUI::w32CompositionEnabled()
{
    BOOL compositionEnabled = false;
    const HRESULT queryComposition = ::DwmIsCompositionEnabled(&compositionEnabled);

    return compositionEnabled && (queryComposition == S_OK);
}


DWORD EDVoiceGUI::w32Style()
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


void EDVoiceGUI::w32CreateWindow(int nShowCmd)
{
    WNDCLASSEXW wcx{};
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.hInstance = nullptr;
    wcx.lpfnWndProc = EDVoiceGUI::w32WndProc;
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
        _mainScale * 1280, _mainScale * 720,
        nullptr,
        nullptr,
        nullptr,
        this
    );

    if (w32CompositionEnabled()) {
        static const MARGINS shadow_state[2]{ { 0,0,0,0 },{ 1,1,1,1 } };
        ::DwmExtendFrameIntoClientArea(_hwnd, &shadow_state[_borderlessWindow ? 1 : 0]);
    }

    ::SetWindowPos(_hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
    ::ShowWindow(_hwnd, nShowCmd);
    ::UpdateWindow(_hwnd);
}


void EDVoiceGUI::w32SetBorderless(bool borderless)
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


bool EDVoiceGUI::w32IsMaximized()
{
    WINDOWPLACEMENT placement;
    if (!::GetWindowPlacement(_hwnd, &placement)) {
        return false;
    }

    return placement.showCmd == SW_MAXIMIZE;
}


void EDVoiceGUI::w32AdjustMaximizedClientRect(RECT& rect)
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


LRESULT EDVoiceGUI::w32HitTest(POINT cursor) const
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
