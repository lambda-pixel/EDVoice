add_executable(EDVoice
    main.cpp
    EliteFileUtil.cpp
    StatusWatcher.cpp
    JournalWatcher.cpp 
    VoicePack.cpp
    EventLogger.cpp
    ../assets/edvoice.rc
)

target_include_directories(EDVoice PRIVATE ../3rdparty)

install(TARGETS EDVoice
        RUNTIME DESTINATION .)