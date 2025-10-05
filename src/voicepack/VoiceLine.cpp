#include "VoiceLine.h"
#include <cassert>
#include <cstdint>

#include "../util/EliteFileUtil.h"


VoiceLine::VoiceLine()
{}


VoiceLine::VoiceLine(const std::filesystem::path& basePath, const nlohmann::json& json)
    : VoiceLine()
{
    loadFromJson(basePath, json);
}


int VoiceLine::getCooldownMs() const
{
    return _cooldownMs;
}


bool VoiceLine::empty() const
{
    return _filepath.size() == 0;
}


std::optional<std::filesystem::path> VoiceLine::getNextVoiceline()
{
    _hasBeenPlayedOnce = true;
    _lastPlayed = std::chrono::steady_clock::now();

    if (_filepath.size() == 0) {
        // This is an error, shall not happen, silently ignore
        assert(0);
        return {};
    }

    if (_filepath.size() == 1) {
        return _filepath[0];
    }

    const float sample = (float)std::rand() / (float)RAND_MAX;

    // Use CDF to select a file
    for (size_t i = 0; i < _cdf.size(); i++) {
        if (sample <= _cdf[i]) {
            return _filepath[i];
        }
    }

    assert(0);

    // Fallback, should not reach here
    return _filepath.back();
}


void VoiceLine::loadFromJson(const std::filesystem::path& basePath, const nlohmann::json& json)
{
    _filepath.clear();
    _probabilities.clear();
    _cdf.clear();
    _cooldownMs = 0;

    // Simple case: backward compatibility with single string
    if (json.is_string()) {
        _filepath.push_back(EliteFileUtil::resolvePath(basePath,json.get<std::string>()));
        _probabilities.push_back(1.0f);
    }
    else if (json.is_object()) {
        if (json.contains("files") && json["files"].is_array()) {
            for (const auto& f : json["files"]) {

                float currProb = 1.0f;
                std::string filename;

                if (f.is_string()) {
                    // Just a list of files
                    filename = f.get<std::string>();
                }
                else if (f.is_object() && f.contains("file") && f["file"].is_string()) {
                    // "file" is specified, check for probability
                    filename = f["file"].get<std::string>();

                    if (f.contains("probability") && f["probability"].is_number()) {
                        const float p = f["probability"].get<float>();
                        currProb = (p > 0.0f) ? p : 1.0f;
                    }
                }

                _filepath.push_back(EliteFileUtil::resolvePath(basePath, filename));
                _probabilities.push_back(currProb);
            }

            if (json.contains("cooldown") && json["cooldown"].is_number_integer()) {
                const int cd = json["cooldown"].get<int>();
                if (cd >= 0) {
                    _cooldownMs = cd;
                }
            }
        }
    }

    computeCDF();
}


void VoiceLine::saveToJson(nlohmann::json& json) const
{
    if (_filepath.size() == 1 && _probabilities.size() == 1 && _probabilities[0] == 1.0f && _cooldownMs == 0) {
        // Simple case: backward compatibility with single string
        json = _filepath[0].string();
    }
    else {
        json = nlohmann::json::object();
        nlohmann::json files = nlohmann::json::array();

        for (size_t i = 0; i < _filepath.size(); i++) {
            if (_probabilities.size() == _filepath.size() && _probabilities[i] != 1.0f) {
                nlohmann::json fileEntry = nlohmann::json::object();
                fileEntry["file"] = _filepath[i].string();
                fileEntry["probability"] = _probabilities[i];
                files.push_back(fileEntry);
            }
            else {
                files.push_back(_filepath[i].string());
            }
        }

        json["files"] = files;

        if (_cooldownMs > 0) {
            json["cooldown"] = _cooldownMs;
        }
    }
}


bool VoiceLine::removeMissingFiles()
{
    bool recompute = false;

    for (size_t i = 0; i < _filepath.size(); ) {
        if (_filepath[i].empty() || !std::filesystem::exists(_filepath[i])) {
            _filepath.erase(_filepath.begin() + i);

            if (i < _probabilities.size()) {
                _probabilities.erase(_probabilities.begin() + i);
            }
            recompute = true;
        }
        else {
            i++;
        }
    }

    if (recompute) {
        computeCDF();
    }

    return recompute;
}


bool VoiceLine::hasCooledDown() const
{
    if (!_hasBeenPlayedOnce || _cooldownMs == 0.f) {
        return true;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastPlayed).count();

    return elapsedMs >= _cooldownMs;
}


void VoiceLine::computeCDF()
{
    _cdf.clear();
    float sum = 0.0f;

    for (const auto& p : _probabilities) {
        sum += p;
        _cdf.push_back(sum);
    }

    // Normalize CDF
    for (auto& c : _cdf) {
        c /= sum;
    }
}