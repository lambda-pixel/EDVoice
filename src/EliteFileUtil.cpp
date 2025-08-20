#pragma once

#include "EliteFileUtil.h"

#include <shlobj.h>

#ifdef min
#undef min
#endif

bool EliteFileUtil::isJournalFile(const std::filesystem::path& path)
{
    const std::wstring filename = path.filename();

    return filename.rfind(L"Journal.", 0) == 0 && path.extension() == L".log";
}


std::filesystem::path EliteFileUtil::getLatestJournal(const std::filesystem::path& folder)
{
    std::filesystem::path latestFile;
    std::filesystem::file_time_type latestTime = std::filesystem::file_time_type::min();

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            if (isJournalFile(entry.path())) {
                auto ftime = std::filesystem::last_write_time(entry);

                if (ftime > latestTime) {
                    latestTime = ftime;
                    latestFile = entry.path();
                }
            }
        }
    }

    return latestFile;
}


bool EliteFileUtil::isStatusFile(const std::filesystem::path& path)
{
    const std::wstring filename = path.filename();

    return filename == L"Status.json";
}


std::filesystem::path EliteFileUtil::getStatusFile(const std::filesystem::path& folder)
{
    return folder / "Status.json";
}


std::filesystem::path EliteFileUtil::getSavedGamesPath()
{
    PWSTR path = NULL;
    std::wstring result;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &path))) {
        result = path;
    }
    CoTaskMemFree(path);

    return result;
}


std::filesystem::path EliteFileUtil::getUserProfile()
{
    return getSavedGamesPath() / "Frontier Developments" / "Elite Dangerous";
}
