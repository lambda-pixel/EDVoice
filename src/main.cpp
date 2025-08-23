#include <iostream>
#include <string>
#include <filesystem>

#include <Windows.h>

#include "EliteFileUtil.h"
#include "StatusWatcher.h"
#include "JournalWatcher.h"
#include "EventLogger.h"
#include "VoicePack.h"
#include "AudioPlayer.h"

#include <thread>
#include <conio.h>


void fileWatcherThread(
    HANDLE hStop,
    const std::filesystem::path userProfile,
    StatusWatcher& statusWatcher,
    JournalWatcher& journalWatcher)
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
                    statusWatcher.update();
                }
                else if (EliteFileUtil::isJournalFile(filename)) {
                    journalWatcher.update(userProfile / filename);
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


/*
int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
*/
int main(int argc, char* argv[])
{
    std::filesystem::path voicePackFile;

    if (argc < 2) {
        std::cout << "Using default voicepack" << std::endl;
        voicePackFile = std::filesystem::current_path() / "Bean.json";
    }
    else {
        voicePackFile = argv[1];
    }

    const std::filesystem::path userProfile = EliteFileUtil::getUserProfile();

    StatusWatcher status(EliteFileUtil::getStatusFile(userProfile));
    JournalWatcher journal(EliteFileUtil::getLatestJournal(userProfile));

    AudioPlayer player(nullptr);
    VoicePack voicePack(voicePackFile, player);
    status.addListener(&voicePack);
    journal.addListener(&voicePack);

    EventLogger logger;
    status.addListener(&logger);
    journal.addListener(&logger);

    // Monitor any file change in user profile folder
    HANDLE hStop = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    std::thread watcherThread(
        fileWatcherThread,
        hStop,
        userProfile,
        std::ref(status),
        std::ref(journal)
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

    watcherThread.join();

    std::cout << "Goodbye!" << std::endl;

    // DestroyWindow(hwnd);
    
    return 0;
}
