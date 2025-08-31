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

    // Load default config or the one specified in command line
    std::filesystem::path voicePackFile;

    if (argc < 2) {
        std::cout << "Using default voicepack" << std::endl;
        voicePackFile = std::filesystem::current_path() / "Bean.json";
    }
    else {
        voicePackFile = argv[1];
    }

    EDVoiceApp app(voicePackFile);

    app.run();

    return 0;
}
