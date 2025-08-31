#pragma once

#include <filesystem>
#include <PluginInterface.h>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE LibHandle;
#define LoadLib(name) LoadLibraryA(name)
#define GetSym GetProcAddress
#define CloseLib FreeLibrary
#else
#include <dlfcn.h>
typedef void* LibHandle;
#define LoadLib(name) dlopen(name, RTLD_NOW)
#define GetSym dlsym
#define CloseLib dlclose
#endif

#include "StatusWatcher.h"
#include "JournalWatcher.h"

// Temporary, this shall be replaced by a plugin
#include "VoicePack.h"

struct LoadedPlugin {
    LibHandle handle = nullptr;
    PluginCallbacks callbacks{};
    std::string name;
};


class PluginJournalListerner : public JournalListener
{
public:
    PluginJournalListerner(PluginCallbacks* callbacks)
        : _callbacks(callbacks)
    {
    }

    void onJournalEvent(const std::string& event, const std::string& journalEntry) override
    {
        if (_callbacks && _callbacks->onJournalEvent) {
            _callbacks->onJournalEvent(event.c_str(), journalEntry.c_str(), _callbacks->ctx);
        }
    }
private:
    PluginCallbacks* _callbacks;
};


class PluginStatusListener : public StatusListener
{
public:
    PluginStatusListener(PluginCallbacks* callbacks)
        : _callbacks(callbacks)
    {
    }
    void onStatusChanged(StatusEvent event, bool set) override
    {
        if (_callbacks && _callbacks->onStatusChanged) {
            _callbacks->onStatusChanged(event, set ? 1 : 0, _callbacks->ctx);
        }
    }
private:
    PluginCallbacks* _callbacks;
};


class EDVoiceApp
{
public:
    EDVoiceApp(const std::filesystem::path& config);
    virtual ~EDVoiceApp();
    void run();

private:
    void fileWatcherThread(
        HANDLE hStop,
        const std::filesystem::path userProfile);

    void loadPlugin(const std::filesystem::path& path);
    void unloadPlugin(LoadedPlugin& plugin);

private:
    std::vector<LoadedPlugin> _plugins;

    std::vector<PluginJournalListerner> _pluginJournalListeners;
    std::vector<PluginStatusListener> _pluginStatusListeners;

    StatusWatcher _statusWatcher;
    JournalWatcher _journalWatcher;

    // Temporary, this shall be replaced by a plugin
    VoicePack _voicePack;
};