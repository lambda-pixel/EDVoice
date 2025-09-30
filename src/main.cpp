#include <iostream>
#include <filesystem>

#include "EDVoiceApp.h"

#ifdef _WIN32

#include <conio.h>
#include <windows.h>

void run_failback_cli(
    const std::filesystem::path& execPath,
    const std::filesystem::path& configFile);

#ifdef GUI_MODE

// ----------------------------------------------------------------------------
// GUI
// ----------------------------------------------------------------------------

#include <sstream>
#include "GUI/EDVoiceGUI.h"

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

    std::ostringstream local;
    std::cout.rdbuf(local.rdbuf());
    std::cerr.rdbuf(local.rdbuf());

    bool failbackMode = false;

    try {
        EDVoiceGUI gui(execPath, configFile, hInstance, nShowCmd);
        gui.run();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL ] Exception: " << e.what() << std::endl;
        failbackMode = true;
    } catch (...) {
        std::cerr << "[FATAL ] Exception unknown" << std::endl;
        failbackMode = true;
    }

    if (failbackMode) {
        std::ofstream out("EDVoiceCrash.log", std::ios::app);
        out << local.str();
        out.close();

        // Clear WM_QUIT messages
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) continue;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        MessageBoxA(NULL, "Could not load the GUI. A Log file EDVoiceCrash.log was created. Please send it for bug review.\nTrying to launch in failback mode...\no7", "EDVoice", MB_OK | MB_ICONERROR);

        run_failback_cli(execPath, configFile);
    }

    return 0;
}

#else // !GUI_MODE

// ----------------------------------------------------------------------------
// Console mode
// ----------------------------------------------------------------------------

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

    run_failback_cli(execPath, configFile);

    return 0;
}

#endif


LRESULT CALLBACK w32WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CLOSE:
            ::DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


void run_failback_cli(
    const std::filesystem::path& execPath,
    const std::filesystem::path& configFile)
{
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    EDVoiceApp app(execPath, configFile);

    MSG msg;
    HWND hConsole = GetConsoleWindow();

    while (IsWindow(hConsole) && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        hConsole = GetConsoleWindow(); // Update in case the console is closed
    }

    FreeConsole();
}

#else // !_WIN32

#include "GUI/EDVoiceGUI.h"
#include <SDL3/SDL.h>

int main(int argc, char* argv[])
{
    const std::filesystem::path execPath = std::filesystem::path(argv[0]).parent_path();
    const std::filesystem::path configFile = execPath / "config" / "default.json";

    SDL_Init(SDL_INIT_VIDEO);

    try {
        EDVoiceGUI app(execPath, configFile);
        app.run();
    } catch (const std::exception& e) {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Could not start EDVoice",
            e.what(), NULL);
    } catch (...) {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Could not start EDVoice",
            "Unknown error", NULL);}

    SDL_Quit();

    return 0;
}

#endif // _WIN32