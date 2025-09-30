#pragma once

#include <vector>
#include <filesystem>
#include <string>
#include <random>
#include <chrono>

#include <json.hpp>

struct VoiceLine
{
public:
    VoiceLine();
    VoiceLine(const std::filesystem::path& basePath, const nlohmann::json& json);

    int getCooldownMs() const;
    bool empty() const;

    const std::filesystem::path& getNextVoiceline();

    void loadFromJson(const std::filesystem::path& basePath, const nlohmann::json& json);
    void saveToJson(nlohmann::json& json) const;

    bool removeMissingFiles();

    bool hasCooledDown() const;

private:
    void computeCDF();

    std::vector<std::filesystem::path> _filepath;
    std::vector<float> _probabilities;
    std::vector<float> _cdf;

    int _cooldownMs = 0;
    std::chrono::steady_clock::time_point _lastPlayed;
    bool _hasBeenPlayedOnce = false;
};
