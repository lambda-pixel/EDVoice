add_executable(EDVoice
    main.cpp
    StatusWatcher.cpp
    VoicePack.cpp
    JournalWatcher.cpp 
    EliteFileUtil.cpp
    ../assets/edvoice.rc
)

target_include_directories(EDVoice PRIVATE ../3rdparty)
