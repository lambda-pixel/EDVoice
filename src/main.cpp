#include <iostream>
#include <filesystem>

#include "EDVoiceApp.h"

/*
int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
*/
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
