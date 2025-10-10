#include "Window.h"

#ifdef USE_SDL
    #include <SDL3/SDL_vulkan.h>
    #include <backends/imgui_impl_sdl3.h>
#else
    #include <backends/imgui_impl_win32.h>
#endif

#ifdef USE_VULKAN
    #include <backends/imgui_impl_vulkan.h>
#else
    #include <backends/imgui_impl_dx11.h>
#endif

#include "inter.cpp"


Window::Window(
    WindowSystem* sys,
    const std::string& title,
    const std::filesystem::path& config)
    : _gpuAdapter(sys)
    , _sys(sys)
    , _configPath(config)
    , _title(title)
{
}


Window::~Window()
{
    ImGui::SetCurrentContext(_imGuiContext);
    ImGui::SaveIniSettingsToDisk(_configPath.string().c_str());

#ifdef USE_VULKAN
    vkDeviceWaitIdle(_gpuAdapter.getDevice());
    ImGui_ImplVulkan_Shutdown();
#else
    ImGui_ImplDX11_Shutdown();
#endif

#ifdef USE_SDL
    ImGui_ImplSDL3_Shutdown();
#else
    ImGui_ImplWin32_Shutdown();
#endif

    ImGui::DestroyContext(_imGuiContext);
}


void Window::beginFrame()
{
    if (!_gpuInitialized) { return; }

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

    ImGui_ImplSDL3_NewFrame();
#else
    MSG msg;
    while (PeekMessage(&msg, _hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplWin32_NewFrame();
#endif

#ifdef USE_VULKAN
    ImGui_ImplVulkan_NewFrame();
#else
    ImGui_ImplDX11_NewFrame();
    _gpuAdapter.startNewFrame();
#endif

    ImGui::NewFrame();
}


void Window::endFrame()
{
    if (!_gpuInitialized) { return; }

    // No ImGui context swicthing shall happen there,
    // no other context is supposed to happen in between
    // beginFrame() & endFrame()

    assert(_imGuiContext == ImGui::GetCurrentContext());
    ImGui::Render();

#ifdef USE_VULKAN
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    if (!is_minimized)
    {
        VkCommandBuffer commandBuffer = _gpuAdapter.startNewFrame();

        if (commandBuffer != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
            _gpuAdapter.renderFrame();
            _gpuAdapter.presentFrame();
        }
        else {
            refreshResize();
        }
    }
#else
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    _gpuAdapter.renderFrame();
    _gpuAdapter.presentFrame();
#endif
}


const char* Window::windowTitle() const
{
    return _title.c_str();
}

void Window::postInit()
{
    // TODO figure out something cleaner
    _gpuAdapter.initDevice(this);
    _gpuInitialized = true;

    IMGUI_CHECKVERSION();
    _imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(_imGuiContext);

#ifdef USE_SDL
    #ifdef USE_VULKAN
    ImGui_ImplSDL3_InitForVulkan(_sdlWindow);
    #else
    ImGui_ImplSDL3_InitForD3D(_sdlWindow);
    #endif
#else
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

#ifdef USE_VULKAN
#else
    ImGui_ImplDX11_Init(_gpuAdapter.device(), _gpuAdapter.deviceContext());
#endif

    refreshResize();
}


#ifdef USE_VULKAN
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
#endif // USE_VULKAN


void Window::onResize(uint32_t width, uint32_t height)
{
    if (!_gpuInitialized) { return; }

    _gpuAdapter.resized(width, height);

    if (_imGuiInitialized) {
        refreshResize();
    }
}


void Window::refreshResize()
{
    if (!_gpuInitialized) { return; }

    ImGui::SetCurrentContext(_imGuiContext);

#ifdef USE_VULKAN
    if (_imGuiInitialized) {
        ImGui_ImplVulkan_Shutdown();
    }

    ImGui_ImplVulkan_InitInfo init_info = {
        _gpuAdapter.API_VERSION,            // ApiVersion
        _gpuAdapter.getInstance(),          // Instance
        _gpuAdapter.getPhysicalDevice(),    // PhysicalDevice
        _gpuAdapter.getDevice(),            // Device
        _gpuAdapter.getQueueFamily(),       // QueueFamily
        _gpuAdapter.getQueue(),             // Queue
        VK_NULL_HANDLE,                     // DescriptorPool
        IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE, // DescriptorPoolSize
        _gpuAdapter.nImageCount(),          // MinImageCount
        _gpuAdapter.nImageCount(),          // ImageCount
        VK_NULL_HANDLE,                     // PipelineCache (optional)
        _gpuAdapter.getRenderPass(),        // RenderPass
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
#endif

    _imGuiInitialized = true;
}
