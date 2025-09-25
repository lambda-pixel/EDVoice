#pragma once

#include <windows.h>
#include "Vulkan/VkAdapter.h"

#include <filesystem>
#include <thread>
#include "../EDVoiceApp.h"

class EDVoiceGUI
{
public:
    EDVoiceGUI(
        const std::filesystem::path& exec_path,
        const std::filesystem::path& config,
        HINSTANCE hInstance, int nShowCmd);
    ~EDVoiceGUI();

    void run();

private:
    void resize();

    void beginMainWindow();
    void endMainWindow();

    // WIN32 stuff for working with borderless windows
    // see https://github.com/melak47/BorderlessWindow/tree/main
    static LRESULT CALLBACK w32WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool w32CompositionEnabled();
    DWORD w32Style();
    void w32CreateWindow(int nShowCmd);
    void w32SetBorderless(bool borderless);
    bool w32IsMaximized();
    void w32AdjustMaximizedClientRect(RECT& rect);
    LRESULT w32HitTest(POINT cursor) const;

    EDVoiceApp _app;
    VkAdapter _vkAdapter;

    bool _imGuiInitialized = false;
    float _mainScale = 1.f;
    bool _borderlessWindow = true;
    float _titlebarHeight = 32.f;
    float _totalButtonWidth = 3 * 32.f;

    HWND _hwnd;
    HINSTANCE _hInstance;
};