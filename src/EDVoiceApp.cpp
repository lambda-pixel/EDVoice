#include "EDVoiceApp.h"

#include <iostream>
#include <thread>
#include <conio.h>

#include "EliteFileUtil.h"


EDVoiceApp::EDVoiceApp(const std::filesystem::path& config)
    : _statusWatcher(EliteFileUtil::getStatusFile(EliteFileUtil::getUserProfile()))
    , _journalWatcher(EliteFileUtil::getLatestJournal(EliteFileUtil::getUserProfile()))
    // Temporary, this shall be replaced by a plugin
    , _voicePack(config)
{
    // Load install plugins
    const std::filesystem::path pluginDir = std::filesystem::current_path() / "plugins";

    if (std::filesystem::exists(pluginDir) && std::filesystem::is_directory(pluginDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
            if (entry.is_regular_file()) {
                const auto& path = entry.path();

                if (path.extension() == ".dll") {
                    loadPlugin(path);
                }
            }
        }
    }

    // Register plugins
    for (auto& plugin : _plugins) {
        if (plugin.callbacks.onJournalEvent) {
            _pluginJournalListeners.push_back(PluginJournalListerner(&plugin.callbacks));
        }
        if (plugin.callbacks.onStatusChanged) {
            _pluginStatusListeners.push_back(PluginStatusListener(&plugin.callbacks));
        }
    }

    for (auto& listener : _pluginJournalListeners) {
        _journalWatcher.addListener(&listener);
    }

    for (auto& listener : _pluginStatusListeners) {
        _statusWatcher.addListener(&listener);
    }

    // Temporary, will be moved to an extension
    _journalWatcher.addListener(&_voicePack);
    _statusWatcher.addListener(&_voicePack);
}


EDVoiceApp::~EDVoiceApp()
{
    for (auto& plugin : _plugins) {
        unloadPlugin(plugin);
    }
}


void EDVoiceApp::run()
{
    HANDLE hStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    const std::filesystem::path userProfile = EliteFileUtil::getUserProfile();

    std::thread watcherThread(
        &EDVoiceApp::fileWatcherThread,
        this,
        hStop,
        userProfile
    );

    std::cout << "Press any key to exit" << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (_kbhit()) {
            SetEvent(hStop);
            std::cout << "Exiting..." << std::endl;
            break;
        }
    }

    std::cout << "Goodbye!" << std::endl;

    watcherThread.join();

    CloseHandle(hStop);
}


void EDVoiceApp::fileWatcherThread(
    HANDLE hStop,
    const std::filesystem::path userProfile)
{
    HANDLE hDir = CreateFileW(
        userProfile.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERR   ] Cannot open status folder." << std::endl;
        return;
    }

    OVERLAPPED ov{};
    ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (ov.hEvent == NULL) {
        std::cerr << "[ERR   ] CreateEvent failed: " << GetLastError() << std::endl;
        CloseHandle(hDir);
        return;
    }

    char buffer[1024] = { 0 };
    DWORD bytesReturned = 0;

    auto postRead = [&]() {
        ResetEvent(ov.hEvent);

        BOOL ok = ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
            nullptr,
            &ov,
            nullptr
        );

        if (!ok && GetLastError() != ERROR_IO_PENDING) {
            std::cerr << "[ERR   ] ReadDirectoryChangesW failed: " << GetLastError() << std::endl;
        }
        };

    postRead();

    HANDLE handles[2] = { ov.hEvent, hStop };

    while (true) {
        DWORD w = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

        if (w == WAIT_OBJECT_0) {
            // Read completed
            DWORD bytes = 0;
            if (!GetOverlappedResult(hDir, &ov, &bytes, FALSE)) {
                DWORD err = GetLastError();

                if (err == ERROR_OPERATION_ABORTED) {
                    // CancelIoEx called
                    break;
                }
                else {
                    // Try restarting the observer
                    std::cerr << "[ERR   ] GetOverlappedResult failed. err=" << err << std::endl;
                    postRead();
                    continue;
                }
            }

            // Check which files changed
            BYTE* ptr = reinterpret_cast<BYTE*>(buffer);

            while (true) {
                auto* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(ptr);
                std::wstring filename(fni->FileName, fni->FileNameLength / sizeof(WCHAR));

                if (EliteFileUtil::isStatusFile(filename)) {
                    _statusWatcher.update();
                }
                else if (EliteFileUtil::isJournalFile(filename)) {
                    _journalWatcher.update(userProfile / filename);
                }

                if (fni->NextEntryOffset == 0) break;
                ptr += fni->NextEntryOffset;
            }

            // Restart the observer
            postRead();
        }
        else if (w == WAIT_OBJECT_0 + 1) {
            // Request to stop
            CancelIoEx(hDir, &ov);
            WaitForSingleObject(ov.hEvent, INFINITE);
            break;
        }
        else {
            std::cerr << "[ERR   ] WaitForMultipleObjects failed. err=" << GetLastError() << std::endl;
            break;
        }
    }

    CloseHandle(ov.hEvent);
    CloseHandle(hDir);
}



void EDVoiceApp::loadPlugin(const std::filesystem::path& path)
{
    LibHandle lib = LoadLib(path.string().c_str());

    if (!lib) {
        std::cerr << "[ERR   ] Cannot load plugin: " << path << std::endl;
        return;
    }

    auto regFn   = reinterpret_cast<void(*)(PluginCallbacks*)>(GetSym(lib, "registerPlugin"));
    auto unregFn = reinterpret_cast<void(*)()>(GetSym(lib, "unregisterPlugin"));

    if (!regFn || !unregFn) {
        std::cerr << "[ERR   ] Invalid plugin: " << path << std::endl;
        CloseLib(lib);
        return;
    }

    _plugins.push_back(LoadedPlugin{});

    _plugins.back().handle = lib;
    _plugins.back().name = path.filename().string();
    regFn(&_plugins.back().callbacks);

    std::cout << "[INFO  ] Loaded plugin: " << _plugins.back().name << std::endl;
}


void EDVoiceApp::unloadPlugin(LoadedPlugin& plugin)
{
    if (plugin.handle) {
        if (plugin.callbacks.onJournalEvent || plugin.callbacks.onStatusChanged) {
            plugin.callbacks.onJournalEvent = nullptr;
            plugin.callbacks.onStatusChanged = nullptr;
            plugin.callbacks.ctx = nullptr;
        }

        auto unregFn = reinterpret_cast<void(*)()>(GetSym(plugin.handle, "unregisterPlugin"));

        if (unregFn) {
            unregFn();
        }

        CloseLib(plugin.handle);
        plugin.handle = nullptr;
        std::cout << "[INFO  ] Unloaded plugin: " << plugin.name << std::endl;
    }
}