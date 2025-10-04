#include "EliteFileUtil.h"

#ifdef _WIN32
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#ifdef min
#undef min
#endif

bool EliteFileUtil::isJournalFile(const std::filesystem::path& path)
{
#ifdef _WIN32
    const std::wstring filename = path.filename();
#else
    const std::wstring filename = path.filename().wstring();
#endif

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
#ifdef _WIN32
    const std::wstring filename = path.filename();
#else
    const std::wstring filename = path.filename().wstring();
#endif

    return filename == L"Status.json";
}


std::filesystem::path EliteFileUtil::getStatusFile(const std::filesystem::path& folder)
{
    return folder / "Status.json";
}


std::filesystem::path EliteFileUtil::getSavedGamesPath()
{
#ifdef _WIN32
    PWSTR path = NULL;
    std::wstring result;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &path))) {
        result = path;
    }
    CoTaskMemFree(path);
#else
    // TODO Linux
    struct passwd *pw = getpwuid(getuid());
    const std::string homedir = pw->pw_dir;

    std::filesystem::path result = std::filesystem::path(homedir) /    
        ".local"/"share"/"Steam"/"steamapps"/"compatdata"/"359320"/"pfx"/"drive_c"/"users"/"steamuser"/"Saved Games";
#endif

    return result;
}


std::filesystem::path EliteFileUtil::getUserProfile()
{
    return getSavedGamesPath() / "Frontier Developments" / "Elite Dangerous";
}


std::filesystem::path EliteFileUtil::resolvePath(
    const std::filesystem::path& basePath,
    const std::filesystem::path& file)
{
    return file.is_absolute() ? file : (basePath / file);
}