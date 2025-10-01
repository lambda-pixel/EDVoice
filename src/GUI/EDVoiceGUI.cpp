#include "EDVoiceGUI.h"

#include <stdexcept>
#include <imgui.h>

#ifdef _WIN32
    #include <windowsx.h>
    #include <dwmapi.h>
    #include <commdlg.h>

    #include <backends/imgui_impl_win32.h>
    #define VK_USE_PLATFORM_WIN32_KHR
#else
    #include <backends/imgui_impl_sdl3.h>
#endif

#include <backends/imgui_impl_vulkan.h>

const wchar_t CLASS_NAME[] = L"EDVoice";
#ifdef BUILD_MEDICORP
const wchar_t WINDOW_TITLE[] = L"EDVoice - MediCorp Edition";
const char WINDOW_TITLE_STD[] = "EDVoice - MediCorp Edition";
#else
const wchar_t WINDOW_TITLE[] = L"EDVoice";
const char WINDOW_TITLE_STD[] = "EDVoice";
#endif

#include "inter.cpp"

EDVoiceGUI::EDVoiceGUI(
#ifdef _WIN32
    const std::filesystem::path& exec_path,
    const std::filesystem::path& config,
    HINSTANCE hInstance, int nShowCmd)
#else
    const std::filesystem::path& exec_path,
    const std::filesystem::path& config)
#endif
    : _app(new EDVoiceApp(exec_path, config))
    , _vkAdapter()
    , _imGuiIniPath(config.parent_path() / "imgui.ini")
{
#ifdef _WIN32
    // ImGui initialization
    ImGui_ImplWin32_EnableDpiAwareness();
    _mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Win32 stuff
    w32CreateWindow(nShowCmd);
    _vkAdapter.initDevice(hInstance, _hwnd);
#else
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

    _vkAdapter.initDevice(_sdlWindow);

    SDL_SetWindowPosition(_sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetWindowHitTest(_sdlWindow, sdlHitTest, this);
    SDL_ShowWindow(_sdlWindow);
#endif

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;
    ImGui::LoadIniSettingsFromDisk(_imGuiIniPath.string().c_str());

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(_mainScale);
    style.FontScaleDpi = _mainScale;

#ifdef _WIN32
    ImGui_ImplWin32_Init(_hwnd);
#else
    ImGui_ImplSDL3_InitForVulkan(_sdlWindow);
#endif

    ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(
        inter_compressed_data,
        inter_compressed_size,
        _mainScale * 20.f);

    resize();
}


EDVoiceGUI::~EDVoiceGUI()
{
    vkDeviceWaitIdle(_vkAdapter.getDevice());

    ImGui::SaveIniSettingsToDisk(_imGuiIniPath.string().c_str());
    ImGui_ImplVulkan_Shutdown();

#ifdef _WIN32
    ImGui_ImplWin32_Shutdown();

    DestroyWindow(_hwnd);
    UnregisterClassW(CLASS_NAME, _hInstance);
#else
    ImGui_ImplSDL3_Shutdown();
    SDL_DestroyWindow(_sdlWindow);
#endif

    delete _app;
}


void EDVoiceGUI::run()
{
#ifndef _WIN32
    const int TARGET_FPS = 60;
    const int FRAME_DELAY_MS = 1000 / TARGET_FPS;

    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 last = 0;
    double deltaTime = 0;
#endif

    bool quit = false;

    while (!quit)
    {
#ifdef _WIN32
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
#else
        if (deltaTime < FRAME_DELAY_MS) {
            SDL_Delay((Uint32)(FRAME_DELAY_MS - deltaTime));
        }

        SDL_Event event;

        while(SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            switch (event.type) {
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    if (event.window.windowID == SDL_GetWindowID(_sdlWindow)) {
                        quit = true;
                    }
                    break;
                    
                case SDL_EVENT_WINDOW_RESIZED:
                    {
                        int width, height;
                        SDL_GetWindowSizeInPixels(_sdlWindow, &width, &height);
                        _vkAdapter.resized(width, height);
                    }
                    break;

                case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
                    {
                        _mainScale = SDL_GetWindowDisplayScale(_sdlWindow);
                        int width, height;
                        SDL_GetWindowSizeInPixels(_sdlWindow, &width, &height);
                        _vkAdapter.resized(width, height);
                    }
                    break;
                default:
                    break;
            }
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
#endif
        ImGui::NewFrame();

        beginMainWindow();
        voicePackGUI();
        endMainWindow();

        if (_hasError) {
            ImGui::OpenPopup("Error");
        }

        if (ImGui::BeginPopupModal("Error")) {
            ImGui::Text("Changing voicepack failed, index is out of bounds");
            if (ImGui::Button("OK")) {
                _hasError = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

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


void EDVoiceGUI::beginMainWindow()
{
    ImGuiViewport* pViewport = ImGui::GetMainViewport();

    if (_borderlessWindow) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        ImGui::SetNextWindowSize(ImVec2(pViewport->Size.x, _titlebarHeight));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("Title", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoDecoration);

        ImGui::SetScrollY(0.0f);
        ImGui::SetScrollX(0.0f);

        const ImGuiStyle& style = ImGui::GetStyle();
        const float titleMarginLeft = 8.f;
        const float buttonWidth = 55.f;
        const ImVec2 buttonSize(buttonWidth, _titlebarHeight);

        _totalButtonWidth = 3 * buttonWidth;

        // Center title vertically
        ImGui::PushFont(NULL, style.FontSizeBase * 1.2f);
        ImGui::SetCursorPos(ImVec2(titleMarginLeft, .5f * (_titlebarHeight - ImGui::GetFontSize())));
        ImGui::Text(WINDOW_TITLE_STD);
        ImGui::PopFont();

        // Minimize & Resize buttons
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 20));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 50));
        {
            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - 3 * buttonWidth, 0.));
            if (ImGui::Button("-", buttonSize)) { minimizeWindow(); }

            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - 2 * buttonWidth, 0.));
            if (ImGui::Button("+", buttonSize)) { maximizeRestoreWindow(); }
        }
        ImGui::PopStyleColor(3);

        // Close button
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 125));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 50, 50, 200));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 0, 0, 255));
        {
            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - buttonWidth, 0.));
            if (ImGui::Button("x", buttonSize)) { closeWindow(); }
        }
        ImGui::PopStyleColor(3);

        // Title separator
        ImU32 col = ImGui::GetColorU32(ImGuiCol_Separator);

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(0, _titlebarHeight),
            ImVec2(pViewport->Size.x, _titlebarHeight),
            col, 1.0f
        );

        ImGui::End();
        ImGui::PopStyleVar(3);

        ImGui::SetNextWindowSize(ImVec2(pViewport->Size.x, pViewport->Size.y - _titlebarHeight));
        ImGui::SetNextWindowPos(ImVec2(0, _titlebarHeight));
    }
    else {
        ImGui::SetNextWindowSize(pViewport->Size);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
    }

    ImGui::Begin("Main", nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize);
 }


void EDVoiceGUI::endMainWindow()
{
    ImGui::End();
}


void EDVoiceGUI::voicePackGUI()
{
    VoicePackManager& voicepack = _app->getVoicepack();

    size_t selectedVoicePackIdx = voicepack.getCurrentVoicePackIndex();

#ifdef BUILD_MEDICORP
    const char* defaultVoicepackLabel = "Standard Voicepack";
#else
    const char* defaultVoicepackLabel = "Current Voicepack";
#endif
    ImGui::Text("%s", defaultVoicepackLabel);
    ImGui::SameLine();
    if (ImGui::BeginCombo(
            "##ComboVoicePack",
            voicepack.getInstalledVoicePacks()[selectedVoicePackIdx].c_str(),
            ImGuiComboFlags_WidthFitPreview)) {
        for (size_t n = 0; n < voicepack.getInstalledVoicePacks().size(); n++) {
            const bool is_selected = (selectedVoicePackIdx == (int)n);

            if (ImGui::Selectable(voicepack.getInstalledVoicePacks()[n].c_str(), is_selected)) {
                selectedVoicePackIdx = (int)n;

                try {
                    voicepack.loadVoicePackByIndex(selectedVoicePackIdx);
                }
                catch (const std::runtime_error& e) {
                    _logErrStr = e.what();
                    _hasError = true;
                }
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();

    if (ImGui::Button("Add")) {
        voicePackOpenDialogGUI();
    }

    float volume = voicepack.getVolume();
    ImGui::SliderFloat("Volume", &volume, 0.f, 1.f, "%.2f");

    if (volume != voicepack.getVolume()) {
        voicepack.setVolume(volume);
    }

#ifdef BUILD_MEDICORP
    ImGui::Text("MediCorp Compliant: %s", _app.getVoicepack().isAltaCompliant() ? "Yes" : "No");
#endif

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Status event voicelines", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Status");
        voicePackStatusGUI();
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Special events voicelines", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Special");
        voicePackSpecialEventGUI();
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Journal events voicelines", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Journal");
        voicePackJourmalEventGUI();
        ImGui::PopID();
    }
}


void EDVoiceGUI::voicePackStatusGUI()
{
    VoicePackManager& voicepack = _app->getVoicepack();

    const std::array<std::array<VoiceTriggerStatus, 2 * StatusEvent::N_StatusEvents>, N_Vehicles>& statusActive = voicepack.getVoiceStatusActive();
    static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter;

    if (ImGui::BeginTable("Status", N_Vehicles + 1, flags)) {
        for (uint32_t iVehicle = 0; iVehicle < N_Vehicles; iVehicle++) {
            ImGui::TableSetupColumn(prettyPrintVehicle((Vehicle)iVehicle));
        }

        ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (uint32_t iEvent = 0; iEvent < StatusEvent::N_StatusEvents; iEvent++) {
            for (int iActivating = 0; iActivating < 2; iActivating++) {
                const bool statusState = iActivating == 1;

                // Check if any event are populated
                bool hasVoiceForOneVehicle = false;

                for (uint32_t iVehicle = 0; iVehicle < N_Vehicles; iVehicle++) {
                    const VoiceTriggerStatus triggerStatus = statusActive[iVehicle][2 * iEvent + iActivating];
                    hasVoiceForOneVehicle = hasVoiceForOneVehicle || (triggerStatus == Active || triggerStatus == Inactive);
                }

                // At least one voice was found for a given vehicle
                if (hasVoiceForOneVehicle) {
                    ImGui::TableNextRow();

                    for (uint32_t iVehicle = 0; iVehicle < N_Vehicles; iVehicle++) {
                        const VoiceTriggerStatus triggerStatus = statusActive[iVehicle][2 * iEvent + iActivating];

                        ImGui::TableNextColumn();

                        if (triggerStatus != Undefined && triggerStatus != MissingFile) {
                            bool active = triggerStatus == Active;
                            uint32_t uid = 2 * (iVehicle * StatusEvent::N_StatusEvents + iEvent) + iActivating;

                            ImGui::PushID(uid);
                            ImGui::Checkbox("", &active);

                            // Change of status
                            if (active != (triggerStatus == Active)) {
                                voicepack.setVoiceStatusState(
                                    (Vehicle)iVehicle,
                                    (StatusEvent)iEvent,
                                    statusState,
                                    active);
                            }

                            ImGui::PopID();
                        }
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", prettyPrintStatusState((StatusEvent)iEvent, statusState));
                }
            }
        }

        ImGui::EndTable();
    }
}


void EDVoiceGUI::voicePackJourmalEventGUI()
{
    VoicePackManager& voicepack = _app->getVoicepack();

    const std::map<std::string, VoiceTriggerStatus>& eventItems = voicepack.getVoiceJournalActive();

    static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter;
    uint32_t uid = 0;

    if (ImGui::BeginTable("Journal", 2, flags)) {
        ImGui::TableSetupColumn("Active");
        ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& voiceItem : eventItems) {
            const VoiceTriggerStatus triggerStatus = voiceItem.second;

            if (triggerStatus != Undefined && triggerStatus != MissingFile) {
                bool active = triggerStatus == Active;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::PushID(uid++);
                ImGui::Checkbox("", &active);
                ImGui::PopID();

                if (active != (triggerStatus == Active)) {
                    voicepack.setVoiceJournalState(voiceItem.first, active);
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", voiceItem.first.c_str());
            }
        }

        ImGui::EndTable();
    }
}


void EDVoiceGUI::voicePackSpecialEventGUI()
{
    VoicePackManager& voicepack = _app->getVoicepack();

    // TODO: shitty copy paste... but works for now :(
    const std::array<VoiceTriggerStatus, N_SpecialEvents>& eventItems = voicepack.getVoiceSpecialActive();

    static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter;
    uint32_t uid = 0;

    if (ImGui::BeginTable("Journal", 2, flags)) {
        ImGui::TableSetupColumn("Active");
        ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (uint32_t iEvent = 0; iEvent < N_SpecialEvents; iEvent++) {
            const VoiceTriggerStatus triggerStatus = eventItems[iEvent];

            if (triggerStatus != Undefined && triggerStatus != MissingFile) {
                bool active = triggerStatus == Active;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::PushID(uid++);
                ImGui::Checkbox("", &active);
                ImGui::PopID();

                if (active != (triggerStatus == Active)) {
                    voicepack.setVoiceSpecialState((SpecialEvent)iEvent, active);
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", prettyPrintSpecialEvent((SpecialEvent)iEvent));
            }
        }

        ImGui::EndTable();
    }
}


void EDVoiceGUI::voicePackOpenDialogGUI()
{
    VoicePackManager& voicepack = _app->getVoicepack();

#ifdef _WIN32
    const std::string newVoicePack = w32OpenFileName(
        "Select voicepack file",
        "",
        "JSON file\0*.json\0",
        false);

    if (!newVoicePack.empty()) {
        try {
            std::string& voicepackName = std::filesystem::path(newVoicePack).stem().string();
            size_t idxNewVoicePack = voicepack.addVoicePack(voicepackName, newVoicePack);
            voicepack.loadVoicePackByIndex(idxNewVoicePack);
        }
        catch (const std::runtime_error& e) {
            _logErrStr = e.what();
            _hasError = true;
        }
    }
#else
    const SDL_DialogFileFilter filters[] = {
        { "JSON file",  "json" }
    };

    SDL_ShowOpenFileDialog(
        sdlCallbackOpenFile,
        this,
        _sdlWindow,
        filters, 1,
        NULL,
        false);
#endif
}


const char* EDVoiceGUI::prettyPrintStatusState(StatusEvent status, bool activated)
{
    switch (status) {
    case Docked: return activated ? "Ship has docked" : "Ship left dock";
    case Landed: return activated ? "Ship has landed" : "Ship is taking off";
    case LandingGear_Down: return activated ? "Landing gears deployed" : "Landing gears retracted";
    case Shields_Up: return activated ? "Shields actived" : "Shields inactived";
    case Supercruise: return activated ? "Supercruise activated" : "Supercruise deactivated";
    case FlightAssist_Off: return activated ? "Fligh assist off" : "Fligh assist on";
    case Hardpoints_Deployed: return activated ? "Hardpoints deployed" : "Hardpoints retracted";
    case In_Wing: return activated ? "Joining wing" : "Leaving wing";
    case LightsOn: return activated ? "Lights on" : "Lights off";
    case Cargo_Scoop_Deployed: return activated ? "Cargo scoop deployed" : "Cargo scoop retracted";
    case Silent_Running: return activated ? "Silent running activated" : "Silent running deactivated";
    case Scooping_Fuel: return activated ? "Fuel scoop deployed" : "Fuel scoop retracted";
    case Srv_Handbrake: return activated ? "SRV handbrake on" : "SRV handbrake off";
    case Srv_using_Turret_view: return activated ? "SRV in turret view on" : "SRV in turret view off";
    case Srv_Turret_retracted: return activated ? "SRV turret retracted" : "SRV turret deployed";
    case Srv_DriveAssist: return activated ? "SRV drive assist activated" : "SRV drive assist deactivated";
    case Fsd_MassLocked: return activated ? "FSD is masslocked" : "FSD is not masslocked";
    case Fsd_Charging: return activated ? "FSD charging" : "FSD not charging";
    case Fsd_Cooldown: return activated ? "FSD is cooling down" : "FSD is not cooling down";
    case Low_Fuel: return activated ? "Low fuel" : "Fuel OK";
    case Over_Heating: return activated ? "Overheating" : "Cooled down";
    case Has_Lat_Long: return activated ? "Has latitude longitude" : "Does not have latitude longitude";
    case IsInDanger: return activated ? "In danger" : "Left danger";
    case Being_Interdicted: return activated ? "Beeing interdicted" : "Left interdiction";
    case In_MainShip: return activated ? "Getting on main ship" : "Getting off main ship";
    case In_Fighter: return activated ? "Getting on fighter" : "Getting off fighter";
    case In_SRV: return activated ? "Getting in SRV" : "Getting off SRV";
    case Hud_in_Analysis_mode: return activated ? "HUD is in analysis mode" : "HUD is in fight mode";
    case Night_Vision: return activated ? "Night vision activated" : "Night vision deactivated";
    case Altitude_from_Average_radius: return activated ? "Altitude from average radius" : "Altitude not from average radius";
    case fsdJump: return activated ? "FSD jumping" : "FSD jump ended";
    case srvHighBeam: return activated ? "SRV high beams enabled" : "SRV high beams disabled";
    case N_StatusEvents: return "Unknown";
    }

    return "Unknown";
}


const char* EDVoiceGUI::prettyPrintVehicle(Vehicle vehicle)
{
    switch (vehicle) {
    case Ship: return "Ship";
    case SRV: return "SRV";
    case OnFoot: return "On foot";
    case N_Vehicles: return "Unknown";
    }

    return "Unknown";
}


const char* EDVoiceGUI::prettyPrintSpecialEvent(SpecialEvent event)
{
    switch (event) {
    case CargoFull: return "Cargo full";
    case CargoEmpty: return "Cargo empty";
    case CollectPod: return "Collect pod";
    case Activating: return "Activating voicepack";
    case Deactivating: return "Deactivating voicepack";
    case FuelScoopFinished: return "Fuel scoop finished";
    case HullIntegrity_Compromised: return "Hull integrity compromised";
    case HullIntegrity_Critical: return "Hull integrity critical";
    case AutoPilot_Liftoff: return "Autopilot liftoff";
    case AutoPilot_Touchdown: return "Autopilot touchdown";
    case N_SpecialEvents: return "Unknown";
    }

    return "Unknown";
}


#ifdef _WIN32
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
        
            case WM_SIZE: {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                _vkAdapter->resized(width, height);
            }
            break;

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
                ::PostQuitMessage(0);
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
    WINDOWPLACEMENT placement = {};
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


std::string EDVoiceGUI::w32OpenFileName(const char* title, const char* initialDir, const char* filter, bool multiSelect)
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

#else

SDL_HitTestResult SDLCALL EDVoiceGUI::sdlHitTest(SDL_Window *win, const SDL_Point *area, void *data)
{
    EDVoiceGUI* obj = (EDVoiceGUI*)data;
    assert(win == obj->_sdlWindow);

    int width, height;
    SDL_GetWindowSize(obj->_sdlWindow, &width, &height);

    const int borderX = 8; // Horizontal resize border thickness in pixels
    const int borderY = 8; // Vertical resize border thickness in pixels

    int x = area->x;
    int y = area->y;

    enum RegionMask {
        client = 0b0000,
        left   = 0b0001,
        right  = 0b0010,
        top    = 0b0100,
        bottom = 0b1000,
    };

    int result =
        ((x < borderX) ? left : 0) |
        ((x >= (width - borderX)) ? right : 0) |
        ((y < borderY) ? top : 0) |
        ((y >= (height - borderY)) ? bottom : 0);

    std::cout << area->x << " " << area->y;

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
            } else {
                return SDL_HITTEST_NORMAL;
            }
        }
        default:                return SDL_HITTEST_NORMAL;
    }
}


void SDLCALL EDVoiceGUI::sdlCallbackOpenFile(void* userdata, const char* const* filelist, int filter)
{
    EDVoiceGUI* obj = (EDVoiceGUI*)userdata;
    VoicePackManager& voicepack = obj->_app->getVoicepack();

    if (!filelist) {
        obj->_logErrStr = SDL_GetError();
        obj->_hasError = true;
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*filelist) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
        return;
    }

    while (*filelist) {
        const std::string newVoicePack = *filelist;
        const std::string voicepackName = std::filesystem::path(newVoicePack).stem().string();
        const size_t idxNewVoicePack = voicepack.addVoicePack(voicepackName, newVoicePack);
        voicepack.loadVoicePackByIndex(idxNewVoicePack);

        SDL_Log("Full path to selected file: '%s'", *filelist);
        filelist++;
    }
}

#endif


void EDVoiceGUI::minimizeWindow()
{
#ifdef _WIN32
    PostMessage(_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
#else
    SDL_MinimizeWindow(_sdlWindow);
#endif
}


void EDVoiceGUI::maximizeRestoreWindow()
{
#ifdef _WIN32
    PostMessage(_hwnd, WM_SYSCOMMAND, IsZoomed(_hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
#else
    if (_isMaximized) {
        SDL_RestoreWindow(_sdlWindow);
    } else {
        SDL_MaximizeWindow(_sdlWindow);
    }

    _isMaximized = !_isMaximized;
#endif
}


void EDVoiceGUI::closeWindow()
{
#ifdef _WIN32
    PostMessage(_hwnd, WM_CLOSE, 0, 0);
#else
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    event.window.windowID = SDL_GetWindowID(_sdlWindow);
    SDL_PushEvent(&event);
#endif
}