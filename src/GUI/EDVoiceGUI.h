#pragma once

#include <filesystem>
#include <thread>

#include "Window/WindowSystem.h"
#include "Window/WindowMain.h"
#include "Window/WindowOverlay.h"
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

    EDVoiceApp _app;
    WindowSystem* _windowSystem;

    WindowMain* _mainWindow;
    //WindowOverlay* _overlayWindow;

    bool _hasError = false;
    std::string _logErrStr;
};