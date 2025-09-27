#pragma once

#include <vector>
#include <filesystem>
#include <string>
#include <random>

#include <json.hpp>


struct VoiceLine
{
public:
    VoiceLine();
    VoiceLine(const std::filesystem::path& basePath, const nlohmann::json& json);

    int getCooldownMs() const;
    bool empty() const;

    const std::filesystem::path& getRandomFilePath();

    void loadFromJson(const std::filesystem::path& basePath, const nlohmann::json& json);
    void saveToJson(nlohmann::json& json) const;

    bool removeMissingFiles();

private:
    void computeCDF();

    std::vector<std::filesystem::path> _filepath;
    int _cooldownMs = 0;
    std::vector<float> _probabilities;

    std::vector<float> _cdf;
};
