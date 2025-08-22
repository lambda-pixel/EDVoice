
#include <iostream>
#include <string>
#include <filesystem>

#include <Windows.h>

#include "EliteFileUtil.h"
#include "StatusWatcher.h"
#include "JournalWatcher.h"
#include "VoicePack.h"
#include "AudioPlayer.h"

/*
int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
*/
int main(int argc, char* argv[])
{
    const std::filesystem::path userProfile = EliteFileUtil::getUserProfile();
    const std::filesystem::path voicePackConfig = "C:\\Users\\Siegfried\\Desktop\\Bean\\Bean.json";

    AudioPlayer player(nullptr);
    VoicePack voicePack(voicePackConfig, player);
    StatusWatcher status(EliteFileUtil::getStatusFile(userProfile));
    JournalWatcher journal(EliteFileUtil::getLatestJournal(userProfile));

    status.addListener(&voicePack);
    journal.addListener(&voicePack);

    // Monitor any file change in user profile folder

    HANDLE hDir = CreateFileW(
        userProfile.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERR] Cannot open status folder." << std::endl;
        return 1;
    }

    char buffer[1024];
    DWORD bytesReturned;

    //MSG msg;
    //while (GetMessage(&msg, NULL, 0, 0)) {
    while (true) {
        if (ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
            &bytesReturned,
            NULL,
            NULL
        )) {
            FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)buffer;
            std::wstring filename(fni->FileName, fni->FileNameLength / sizeof(WCHAR));

            if (EliteFileUtil::isStatusFile(filename)) {
                status.update();
            }
            else if (EliteFileUtil::isJournalFile(filename)) {
                journal.update(userProfile / filename);
            }
        }

        //TranslateMessage(&msg);
        //DispatchMessage(&msg);
    }

    CloseHandle(hDir);

    // DestroyWindow(hwnd);
    
    return 0;
}
