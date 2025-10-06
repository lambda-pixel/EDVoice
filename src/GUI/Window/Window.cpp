#include "Window.h"

#ifdef USE_SDL
    #include <backends/imgui_impl_sdl3.h>
#else
    #include <backends/imgui_impl_win32.h>
#endif

#include <backends/imgui_impl_vulkan.h>


Window::Window(
    WindowSystem* sys,
    const std::string& title,
    const std::filesystem::path& config)
    : _vkAdapter(sys)
    , _sys(sys)
    , _configPath(config)
    , _title(title)
{
}


Window::~Window()
{
}


void Window::beginFrame()
{
    ImGui::SetCurrentContext(_imGuiContext);

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
    while (PeekMessage(&msg, _hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
#endif

    ImGui::NewFrame();
}


void Window::endFrame()
{
    // No ImGui context swicthing shall happen there,
    // no other context is supposed to happen in between
    // beginFrame() & endFrame()

    assert(_imGuiContext == ImGui::GetCurrentContext());

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
            refreshResize();
        }
    }
}


const char* Window::windowTitle() const
{
    return _title.c_str();
}


void Window::createVkSurfaceKHR(
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
        _sys->_hInstance,                                         // hinstance
        _hwnd                                                     // hwnd
    };

    vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface);

    RECT rect;
    ::GetClientRect(_hwnd, &rect);
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
#endif
}


void Window::onResize(uint32_t width, uint32_t height)
{
    _vkAdapter.resized(width, height);

    if (_imGuiInitialized) {
        refreshResize();
    }
}


void Window::refreshResize()
{
    ImGui::SetCurrentContext(_imGuiContext);

    if (_imGuiInitialized) {
        ImGui_ImplVulkan_Shutdown();
    }

    ImGui_ImplVulkan_InitInfo init_info = {
        _vkAdapter.API_VERSION,            // ApiVersion
        _vkAdapter.getInstance(),          // Instance
        _vkAdapter.getPhysicalDevice(),    // PhysicalDevice
        _vkAdapter.getDevice(),            // Device
        _vkAdapter.getQueueFamily(),       // QueueFamily
        _vkAdapter.getQueue(),             // Queue
        VK_NULL_HANDLE,                     // DescriptorPool
        IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE, // DescriptorPoolSize
        _vkAdapter.nImageCount(),          // MinImageCount
        _vkAdapter.nImageCount(),          // ImageCount
        VK_NULL_HANDLE,                     // PipelineCache (optional)
        _vkAdapter.getRenderPass(),        // RenderPass
        0,                                  // Subpass
        VK_SAMPLE_COUNT_1_BIT,              // msaaSamples
        false,                              // UseDynamicRendering
    #ifdef IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
        {},                                 // PipelineRenderingCreateInfo (optional)
    #endif
        nullptr,                            // VkAllocationCallbacks
        nullptr,                            // (*CheckVkResultFn)(VkResult err)
        1024 * 1024                         // MinAllocationSize
    };

    ImGui_ImplVulkan_Init(&init_info);

    _imGuiInitialized = true;
}

