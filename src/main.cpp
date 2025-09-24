#include <iostream>
#include <filesystem>

#include "EDVoiceApp.h"
#include "GUI/EDVoiceGUI.h"

// ----------------------------------------------------------------------------
// GUI
// ----------------------------------------------------------------------------

#include <windows.h>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    EDVoiceGUI gui(hInstance, nShowCmd);

    gui.run();
    
    return 0;
}

// ----------------------------------------------------------------------------


#if 0
int main(int argc, char* argv[])
{
    const std::filesystem::path execPath = std::filesystem::path(argv[0]).parent_path();
    // Load default config or the one specified in command line
    std::filesystem::path configFile;

    if (argc < 2) {
        std::cout << "Using default configuration" << std::endl;
        configFile = std::filesystem::current_path() / "config" / "default.json";
    }
    else {
        configFile = argv[1];
    }

    EDVoiceApp app(execPath, configFile);

    app.run();

    return 0;
}
#endif