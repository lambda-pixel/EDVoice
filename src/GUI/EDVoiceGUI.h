#pragma once

#include <filesystem>
#include <thread>

#include "Vulkan/VkAdapter.h"
#include "WindowSystem.h"
#include "../EDVoiceApp.h"

class EDVoiceGUI
{
public:
    EDVoiceGUI(
        const std::filesystem::path& exec_path,
        const std::filesystem::path& config,
        WindowSystem* windowSystem);

    ~EDVoiceGUI();

    void run();

private:
    void resize();

    void beginMainWindow();
    void endMainWindow();

    void voicePackGUI();

    void voicePackStatusGUI();
    void voicePackJourmalEventGUI();
    void voicePackSpecialEventGUI();

    static void loadVoicePack(void* userdata, std::string path);

    static const char* prettyPrintStatusState(StatusEvent status, bool activated);
    static const char* prettyPrintVehicle(Vehicle vehicle);
    static const char* prettyPrintSpecialEvent(SpecialEvent event);

    EDVoiceApp* _app;
    VkAdapter _vkAdapter;
    WindowSystem* _windowSystem;

    bool _imGuiInitialized = false;

    bool _hasError = false;
    std::string _logErrStr;

    std::filesystem::path _imGuiIniPath;
};