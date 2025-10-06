#pragma once

#include "Window.h"

#include <config.h>

// The overlay feature is currently Windows specific
#ifndef USE_SDL

class WindowOverlay: public Window
{
public:
    WindowOverlay(
        WindowSystem* sys,
        const std::string& title,
        const std::filesystem::path& config,
        const std::string& processName
    );
};

#endif // !USE_SDL