#pragma once

#include <filesystem>
#include <map>

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

#include "watchers/StatusWatcher.h"
#include "watchers/JournalWatcher.h"

struct LoadedPlugin {
    LibHandle handle = nullptr;
    PluginCallbacks callbacks{};
    std::string name, versionStr, author, longname;
};


class PluginJournalListerner : public JournalListener
{
public:
    PluginJournalListerner(PluginCallbacks* callbacks)
        : _callbacks(callbacks)
    {
    }

    void setJournalPreviousEvent(const std::string& event, const std::string& journalEntry) override
    {
        if (_callbacks && _callbacks->setJournalPreviousEvent) {
            _callbacks->setJournalPreviousEvent(event.c_str(), journalEntry.c_str(), _callbacks->ctx);
        }
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
    EDVoiceApp(
        const std::filesystem::path& exec_path,
        const std::filesystem::path& config);
    virtual ~EDVoiceApp();
    void run();

private:
    void fileWatcherThread(
        HANDLE hStop,
        const std::filesystem::path userProfile);

    void loadPlugin(const std::filesystem::path& path);
    void unloadPlugin(LoadedPlugin& plugin);

private:
    std::map<std::string, std::filesystem::path> _config;
    std::vector<LoadedPlugin> _plugins;

    std::vector<PluginJournalListerner> _pluginJournalListeners;
    std::vector<PluginStatusListener> _pluginStatusListeners;

    StatusWatcher _statusWatcher;
    JournalWatcher _journalWatcher;
};