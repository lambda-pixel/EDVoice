#pragma once

#include <filesystem>


struct EliteFileUtil
{
    static bool isJournalFile(const std::filesystem::path& path);

    static std::filesystem::path getLatestJournal(const std::filesystem::path& folder);

    static bool isStatusFile(const std::filesystem::path& path);

    static std::filesystem::path getStatusFile(const std::filesystem::path& folder);

    static std::filesystem::path getSavedGamesPath();

    static std::filesystem::path getUserProfile();

    static std::filesystem::path resolvePath(
        const std::filesystem::path& basePath,
        const std::filesystem::path& file);
};