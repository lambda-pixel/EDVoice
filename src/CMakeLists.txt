add_executable(EDVoice
    main.cpp
    StatusWatcher.cpp
    VoicePack.cpp
    JournalWatcher.cpp 
    "EliteFileUtil.h" "JournalEvent.h" "StatusEvent.h" "EliteFileUtil.cpp")

target_include_directories(EDVoice PRIVATE ../3rdparty)