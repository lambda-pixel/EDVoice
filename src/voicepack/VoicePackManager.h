#pragma once

#include "VoicePack.h"
#include "AudioPlayer.h"
#include "Enum.h"

#ifdef BUILD_MEDICORP
#include "MedicComlpiant.h"
#endif

#include <filesystem>
#include <string>
#include <map>
#include <array>


class VoicePackManager
{
public:
    VoicePackManager();
    ~VoicePackManager();

    void loadConfig(const char* filepath);
    void saveConfig() const;

    void loadVoicePackByIndex(size_t index);
    size_t addVoicePack(const std::string& name, const std::string& path);

    void onStatusChanged(StatusEvent event, bool status);
    void setJournalPreviousEvent(const std::string& event, const std::string& journalEntry);
    void onJournalEvent(const std::string& event, const std::string& journalEntry);

    VoicePack& getStandardVoicePack() { return _standardVoicePack; }
#ifdef BUILD_MEDICORP
    VoicePack& getMedicVoicePack() { return _medicVoicePack; }
    bool isAltaCompliant() const { return _medicCompliant.isCompliant(); }
#endif

    static size_t indexFromStatusEvent(StatusEvent event, bool status) {
        return 2 * event + (status ? 1 : 0);
    }

    void playStatusVoiceline(Vehicle vehicle, StatusEvent event, bool status, const std::filesystem::path& path);
    void playJournalVoiceline(const std::string& event, const std::filesystem::path& path);
    void playSpecialVoiceline(SpecialEvent event, const std::filesystem::path& path);

    const std::array<std::array<VoiceTriggerStatus, 2 * StatusEvent::N_StatusEvents>, N_Vehicles>& getVoiceStatusActive() const { return _configVoiceStatusActive; }
    const std::map<std::string, VoiceTriggerStatus>& getVoiceJournalActive() const { return _configVoiceJournalActive; }
    const std::array<VoiceTriggerStatus, N_SpecialEvents>& getVoiceSpecialActive() const { return _configVoiceSpecialActive; }

    void setVoiceStatusState(Vehicle vehicle, StatusEvent event, bool statusState, bool active);
    void setVoiceJournalState(const std::string& event, bool active);
    void setVoiceSpecialState(SpecialEvent event, bool active);

    const std::vector<std::string>& getInstalledVoicePacks() const { return _installedVoicePacksNames; }
    size_t getCurrentVoicePackIndex() const { return _currentVoicePackIndex; }

private:
    void updateVoicePackSettings(VoicePack& voicepack);

    std::filesystem::path _configPath;

    VoicePack _standardVoicePack;

    // MediCorp specific ALTA voicepack
#ifdef BUILD_MEDICORP
    MedicCompliant _medicCompliant;
    bool _altaActive = false;
    VoicePack _medicVoicePack;
#endif

    // List of installed voicepacks:
    // name, path (the same string as in the config)
    std::map<std::string, std::string> _installedVoicePacks;
    std::map<std::string, std::filesystem::path> _installedVoicePacksAbsolutePath;

    // Everything except the MedicCorp ALTA voicepack
    std::vector<std::string> _installedVoicePacksNames;
    size_t _currentVoicePackIndex = 0;

    // As determined by the config file
    std::array<std::array<VoiceTriggerStatus, 2 * StatusEvent::N_StatusEvents>, N_Vehicles> _configVoiceStatusActive;
    std::map<std::string, VoiceTriggerStatus> _configVoiceJournalActive;
    std::array<VoiceTriggerStatus, N_SpecialEvents> _configVoiceSpecialActive;

    AudioPlayer _player;

    bool _isShutdownState = false;
    bool _isPriming = false;
};


//DECLARE_PLUGIN(VoicePackManager, "VoicePack", "0.1", "Siegfried-Origin")
