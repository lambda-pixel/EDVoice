#include "EDVoiceApp.h"

#include <iostream>
#ifdef _WIN32
#else
    #include <sys/inotify.h>
    #include <unistd.h>
    #include <limits.h>
#endif

#include <json.hpp>

#include "util/EliteFileUtil.h"
#define __STDC_WANT_LIB_EXT1__ 1
#include <cstring>

#ifdef _WIN32
    #include <conio.h>
#else
#include <termios.h>
#include <fcntl.h>

int kbhit() {
    termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

#endif

// Ugly hack for now but whatever...
static void loadConfigVP(const char* filepath, void* ctx)
{
    reinterpret_cast<VoicePackManager*>(ctx)->loadConfig(filepath);
}

static void onStatusChangedVP(StatusEvent event, int set, void* ctx)
{
    reinterpret_cast<VoicePackManager*>(ctx)->onStatusChanged(event, set);
}

static void setJournalPreviousEventVP(const char* event, const char* jsonEntry, void* ctx)
{
    reinterpret_cast<VoicePackManager*>(ctx)->setJournalPreviousEvent(event, jsonEntry);
}

static void onJournalEventVP(const char* event, const char* jsonEntry, void* ctx)
{
    reinterpret_cast<VoicePackManager*>(ctx)->onJournalEvent(event, jsonEntry);
}

extern "C" {
    void registerPluginVP(VoicePackManager* voicepack, PluginCallbacks* callbacks) {
        callbacks->loadConfig = loadConfigVP;
        callbacks->onStatusChanged = onStatusChangedVP;
        callbacks->setJournalPreviousEvent = setJournalPreviousEventVP;
        callbacks->onJournalEvent = onJournalEventVP;
        callbacks->ctx = voicepack;

        std::strncpy(callbacks->name, "VoicePack", sizeof(callbacks->name) - 1);
        std::strncpy(callbacks->versionStr, "0.3", sizeof(callbacks->name) - 1);
        std::strncpy(callbacks->author, "Siegfried-Origin", sizeof(callbacks->name) - 1);
    }
    void unregisterPluginVP()
    {
    }
}


EDVoiceApp::EDVoiceApp(
    const std::filesystem::path& exec_path,
    const std::filesystem::path& config)
    : _statusWatcher(EliteFileUtil::getStatusFile(EliteFileUtil::getUserProfile()))
    , _journalWatcher(EliteFileUtil::getLatestJournal(EliteFileUtil::getUserProfile()))
{
    // Load configuration
    if (!std::filesystem::exists(config)) {
        throw std::runtime_error("Cannot find configuration file: " + config.string());
    }

    std::filesystem::path basePath;

    if (config.is_absolute()) {
        basePath = config.parent_path();
    }
    else {
        basePath = std::filesystem::current_path() / config.parent_path();
    }

    std::cout << "[INFO  ] Using configuration file: " << config << std::endl;
    {
        std::ifstream configFile(config);
        std::string fileContent((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());

        nlohmann::json json = nlohmann::json::parse(fileContent);

        if (json.contains("plugins")) {
            for (auto& item : json["plugins"].items()) {
                for (auto& pluginItem : item.value().items()) {
                    if (pluginItem.key() == "config") {
                        _config[item.key()] = basePath / pluginItem.value().get<std::string>();
                    }
                }
            }
        }
        // Right now, we keep loading voicepacks even if they are not in the "plugins" section
        else if (json.contains("status") || json.contains("event")) {
            std::cout << "[WARN  ] Configuration file seems to be a voicepack. We will force load it but this behavior may be dropped in future versions." << std::endl;
            _config["VoicePack"] = config;
        }
    }

    // Load installed plugins
    const std::filesystem::path pluginDir = exec_path / "plugins";

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

    //// Now, register manually the voicepack
    _plugins.push_back(LoadedPlugin{});
    LoadedPlugin& voice = _plugins.back();
    voice.handle = nullptr;
    voice.name = "VoicePack";
    voice.author = "Siegfried-Origin";
    voice.versionStr = "0.3";
    registerPluginVP(&_voicepack, &voice.callbacks);

    // Register plugins
    for (auto& plugin : _plugins) {
        // if configuration exists for this plugin, load it
        auto it = _config.find(plugin.name);

        if (plugin.callbacks.loadConfig && it != _config.end()) {
            const std::filesystem::path& cfgPath = it->second;
            std::cout << "[INFO  ] Loading config for plugin " << plugin.name << ": " << cfgPath << std::endl;
            plugin.callbacks.loadConfig(cfgPath.string().c_str(), plugin.callbacks.ctx);
        }

        if (plugin.callbacks.onJournalEvent) {
            std::cout << "[INFO  ] Registering journal listener for plugin " << plugin.name << std::endl;
            _pluginJournalListeners.push_back(PluginJournalListerner(&plugin.callbacks));
        }
        if (plugin.callbacks.onStatusChanged) {
            std::cout << "[INFO  ] Registering status listener for plugin " << plugin.name << std::endl;
            _pluginStatusListeners.push_back(PluginStatusListener(&plugin.callbacks));
        }
    }

    for (auto& listener : _pluginJournalListeners) {
        _journalWatcher.addListener(&listener);
    }

    for (auto& listener : _pluginStatusListeners) {
        _statusWatcher.addListener(&listener);
    }

    // Prime watchers
    _journalWatcher.start();
    _statusWatcher.start();

    // Start monitoring file change
#ifdef _WIN32
    _hStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    _watcherThread = std::thread(
        &EDVoiceApp::fileWatcherThread,
        this,
        _hStop);
#else
    _watcherThread = std::thread(
        &EDVoiceApp::fileWatcherThread,
        this
    );
#endif
}


EDVoiceApp::~EDVoiceApp()
{
    for (auto& plugin : _plugins) {
        unloadPlugin(plugin);
    }

#ifdef _WIN32
    SetEvent(_hStop);
    _watcherThread.join();
    CloseHandle(_hStop);
#else
    _hStop = false;
    _watcherThread.join();
#endif
}


void EDVoiceApp::run()
{
    std::cout << "Press any key to exit" << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

#ifdef _WIN32
        if (_kbhit()) {
            SetEvent(_hStop);
            std::cout << "Exiting..." << std::endl;
            break;
        }
#else
        if (kbhit()) {
            _hStop = true;
            std::cout << "Exiting..." << std::endl;
            break;
        }
#endif
    }

    std::cout << "Goodbye!" << std::endl;
}


#ifdef _WIN32
void EDVoiceApp::fileWatcherThread(HANDLE hStop)
{
    const std::filesystem::path userProfile = EliteFileUtil::getUserProfile();

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
                else {
                    std::wcout << L"[INFO  ] Ignoring file change: " << filename << std::endl;
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
#else
void EDVoiceApp::fileWatcherThread()
{
    while (!_hStop) {
        // TODO Linux
    }
}
#endif


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

    if (_plugins.back().callbacks.name[0] != '\0') {
        _plugins.back().name     = _plugins.back().callbacks.name;
        _plugins.back().longname = _plugins.back().callbacks.name;
    }
    if (_plugins.back().callbacks.versionStr[0] != '\0') {
        _plugins.back().versionStr = _plugins.back().callbacks.versionStr;
        _plugins.back().longname += " v";
        _plugins.back().longname += _plugins.back().callbacks.versionStr;
    }
    if (_plugins.back().callbacks.author[0] != '\0') {
        _plugins.back().author = _plugins.back().callbacks.author;
        _plugins.back().longname += " by ";
        _plugins.back().longname += _plugins.back().callbacks.author;
    }

    std::cout << "[INFO  ] Loaded plugin: " << path.filename() << " - " << _plugins.back().longname << std::endl;
}


void EDVoiceApp::unloadPlugin(LoadedPlugin& plugin)
{
    if (plugin.handle) {
        plugin.callbacks.onJournalEvent = nullptr;
        plugin.callbacks.onStatusChanged = nullptr;
        plugin.callbacks.loadConfig = nullptr;
        plugin.callbacks.name[0] = '\0';
        plugin.callbacks.versionStr[0] = '\0';
        plugin.callbacks.author[0] = '\0';
        plugin.callbacks.ctx = nullptr;

        auto unregFn = reinterpret_cast<void(*)()>(GetSym(plugin.handle, "unregisterPlugin"));

        if (unregFn) {
            unregFn();
        }

        CloseLib(plugin.handle);
        plugin.handle = nullptr;
        std::cout << "[INFO  ] Unloaded plugin: " << plugin.name << std::endl;
    }
}