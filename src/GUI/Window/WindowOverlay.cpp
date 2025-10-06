#include "WindowOverlay.h"


WindowOverlay(
    WindowSystem* sys,
    const std::string& title,
    const std::filesystem::path& config,
    const std::string& processName)
    : Window(sys, title, config)
{

}