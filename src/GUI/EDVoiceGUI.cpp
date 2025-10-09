#include "EDVoiceGUI.h"

#include <stdexcept>
#include <imgui.h>


// TODO: remove
#ifdef BUILD_MEDICORP
const char* WINDOW_TITLE = "EDVoice - MediCorp Edition";
#else
const char* WINDOW_TITLE = "EDVoice";
#endif


EDVoiceGUI::EDVoiceGUI(
    const std::filesystem::path& exec_path,
    const std::filesystem::path& config,
    WindowSystem* windowSystem)
    : _app(exec_path, config)
    , _windowSystem(windowSystem)
{
    _mainWindow = new WindowMain(
        windowSystem,
        WINDOW_TITLE,
        config.parent_path() / "imgui.ini"
    );

    _overlayWindow = new WindowOverlay(
        windowSystem,
        "EDVoice overlay",
        config.parent_path() / "overlay.ini",
        L"EliteDangerous64.exe"
    );
}


EDVoiceGUI::~EDVoiceGUI()
{
    delete _mainWindow;
    //delete _overlayWindow;
}


void EDVoiceGUI::run()
{
    while (!_mainWindow->closed())
    {
        _mainWindow->beginFrame();

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

        _mainWindow->endFrame();

        _overlayWindow->beginFrame();
        ImGui::ShowDemoWindow();
        _overlayWindow->endFrame();

        _windowSystem->collectEvents();
    }
}


void EDVoiceGUI::beginMainWindow()
{
    ImGuiViewport* pViewport = ImGui::GetMainViewport();

    if (_mainWindow->borderlessWindow()) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        ImGui::SetNextWindowSize(ImVec2(pViewport->Size.x, _mainWindow->titleBarHeight()));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("Title", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoDecoration);

        ImGui::SetScrollY(0.0f);
        ImGui::SetScrollX(0.0f);

        const ImGuiStyle& style = ImGui::GetStyle();
        const float titleMarginLeft = 8.f;
        const float buttonWidth = _mainWindow->windowButtonWidth();
        const ImVec2 buttonSize(buttonWidth, _mainWindow->titleBarHeight());

        // Center title vertically
        ImGui::PushFont(NULL, style.FontSizeBase * 1.2f);
        ImGui::SetCursorPos(ImVec2(titleMarginLeft, .5f * (_mainWindow->titleBarHeight() - ImGui::GetFontSize())));
        ImGui::Text("%s", _mainWindow->windowTitle());
        ImGui::PopFont();

        // Minimize & Resize buttons
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 20));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 50));
        {
            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - 3 * buttonWidth, 0.));
            if (ImGui::Button("-", buttonSize)) { _mainWindow->minimizeWindow(); }

            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - 2 * buttonWidth, 0.));
            if (ImGui::Button("+", buttonSize)) { _mainWindow->maximizeRestoreWindow(); }
        }
        ImGui::PopStyleColor(3);

        // Close button
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 125));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 50, 50, 200));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 0, 0, 255));
        {
            ImGui::SetCursorPos(ImVec2(pViewport->Size.x - buttonWidth, 0.));
            if (ImGui::Button("x", buttonSize)) { _mainWindow->closeWindow(); }
        }
        ImGui::PopStyleColor(3);

        // Title separator
        ImU32 col = ImGui::GetColorU32(ImGuiCol_Separator);

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(0, _mainWindow->titleBarHeight()),
            ImVec2(pViewport->Size.x, _mainWindow->titleBarHeight()),
            col, 1.0f
        );

        ImGui::End();
        ImGui::PopStyleVar(3);

        ImGui::SetNextWindowSize(ImVec2(pViewport->Size.x, pViewport->Size.y - _mainWindow->titleBarHeight()));
        ImGui::SetNextWindowPos(ImVec2(0, _mainWindow->titleBarHeight()));
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
    VoicePackManager& voicepack = _app.getVoicepack();

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
        _mainWindow->openVoicePackFileDialog(this, EDVoiceGUI::loadVoicePack);
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
    VoicePackManager& voicepack = _app.getVoicepack();

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
    VoicePackManager& voicepack = _app.getVoicepack();

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
    VoicePackManager& voicepack = _app.getVoicepack();

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


void EDVoiceGUI::loadVoicePack(void* userdata, std::string path)
{
    EDVoiceGUI* obj = (EDVoiceGUI*)userdata;
    VoicePackManager& voicepack = obj->_app.getVoicepack();

    if (!path.empty()) {
        try {
            const std::string voicepackName = std::filesystem::path(path).stem().string();
            size_t idxNewVoicePack = voicepack.addVoicePack(voicepackName, path);
            voicepack.loadVoicePackByIndex(idxNewVoicePack);
        }
        catch (const std::runtime_error& e) {
            obj->_logErrStr = e.what();
            obj->_hasError = true;
        }
    }
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
