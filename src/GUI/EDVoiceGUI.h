#pragma once

#include <windows.h>
#include "Vulkan/VkAdapter.h"


class EDVoiceGUI
{
public:
    EDVoiceGUI(HINSTANCE hInstance, int nShowCmd);
    ~EDVoiceGUI();

    void run();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:

    void resize();

    bool _imGuiInitialized = false;

    VkAdapter _vkAdapter;

    HWND _hwnd;
    HINSTANCE _hInstance;
};