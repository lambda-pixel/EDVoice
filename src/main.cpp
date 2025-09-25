#include <iostream>
#include <filesystem>

#include "EDVoiceApp.h"
#include "GUI/EDVoiceGUI.h"

// ----------------------------------------------------------------------------
// GUI
// ----------------------------------------------------------------------------

#if 1
#include <windows.h>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    LPWSTR* szArgList;
    int argCount;
    szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);

    const std::filesystem::path execPath = std::filesystem::path(szArgList[0]).parent_path();
    const std::filesystem::path configFile = execPath / "config" / "default.json";

    EDVoiceGUI gui(execPath, configFile, hInstance, nShowCmd);

    gui.run();
    
    return 0;
}

// ----------------------------------------------------------------------------

#else

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