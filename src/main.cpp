#include <iostream>
#include <filesystem>
#include <sstream>

#include "EDVoiceApp.h"

#if !defined(GUI_MODE) && defined(WIN32)

// ----------------------------------------------------------------------------
// WIN32 console mode
// ----------------------------------------------------------------------------

#include <conio.h>
#include <windows.h>

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

#else

// ----------------------------------------------------------------------------
// GUI mode
// ----------------------------------------------------------------------------

#include "GUI/EDVoiceGUI.h"
#include "GUI/WindowSystem.h"

#ifdef WIN32
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    LPWSTR* argv;
    int argCount;
    argv = CommandLineToArgvW(GetCommandLine(), &argCount);
#endif
#ifndef WIN32
int main(int argc, char* argv[])
{
#endif
    const std::filesystem::path execPath = std::filesystem::path(argv[0]).parent_path();
    const std::filesystem::path configFile = execPath / "config" / "default.json";

    std::ostringstream local;
    std::cout.rdbuf(local.rdbuf());
    std::cerr.rdbuf(local.rdbuf());

    bool failbackMode = false;

    try {
#ifdef USE_SDL
        WindowSystem windowSystem;
#else
        WindowSystem windowSystem(hInstance, nShowCmd);
#endif
        EDVoiceGUI gui(execPath, configFile, &windowSystem);
        gui.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL ] Exception: " << e.what() << std::endl;
        failbackMode = true;

        // TODO:
        // SDL_ShowSimpleMessageBox(
        //    SDL_MESSAGEBOX_ERROR,
        //    "Could not start EDVoice",
        //    e.what(), NULL);
    }
    catch (...) {
        std::cerr << "[FATAL ] Exception unknown" << std::endl;
        failbackMode = true;
    }

    if (failbackMode) {
        std::ofstream out("EDVoiceCrash.log", std::ios::app);
        out << local.str();
        out.close();
    }

    return 0;
}

#endif // GUI_MODE || !WIN32
