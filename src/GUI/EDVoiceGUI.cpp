#include "EDVoiceGUI.h"

#include <windows.h>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <backends/imgui_impl_vulkan.h>


const wchar_t CLASS_NAME[] = L"EDVoice";


EDVoiceGUI::EDVoiceGUI(HINSTANCE hInstance, int nShowCmd)
    : _vkAdapter({ VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface" })
{    
    // ImGui initialization
    ImGui_ImplWin32_EnableDpiAwareness();
    const float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = EDVoiceGUI::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassExW(&wc);

    _hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"EDVoice",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        main_scale * 1280, main_scale * 720,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    _vkAdapter.initDevice(hInstance, _hwnd);

    ShowWindow(_hwnd, nShowCmd);
    UpdateWindow(_hwnd);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    ImGui::StyleColorsDark();

    // Scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplWin32_Init(_hwnd);
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

        ImGui::ShowDemoWindow(&show_demo_window);

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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);                // Use ImGui::GetCurrentContext()

LRESULT CALLBACK EDVoiceGUI::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg)
    {
        case WM_SIZE:
            // _vkAdapter != VK_NULL_HANDLE &&
            if (wParam != SIZE_MINIMIZED)
            {
                // Resize swap chain
                const int fb_width = (UINT)LOWORD(lParam);
                const int fb_height = (UINT)HIWORD(lParam);

                // TODO!
                //if (fb_width > 0 && fb_height > 0 &&
                //    (g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height))
                //{
                //    //ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
                //    //ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount);
                //    g_MainWindowData.FrameIndex = 0;
                //    g_SwapChainRebuild = false;
                //}
            }
            return 0;
        //case WM_SYSCOMMAND:
        //    if ((wParam & 0xfff0) == SC_KEYMENU) {
        //        // Disable ALT application menu
        //        return 0;
        //    }
        //    break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
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