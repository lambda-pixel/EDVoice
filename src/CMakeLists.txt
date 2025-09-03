add_executable(EDVoice
    main.cpp
    EDVoiceApp.cpp
    EliteFileUtil.cpp
    StatusWatcher.cpp
    JournalWatcher.cpp 
    ../assets/edvoice.rc
)

target_include_directories(EDVoice PRIVATE ../3rdparty)
target_include_directories(EDVoice PRIVATE ../plugins/include)

install(TARGETS EDVoice
        RUNTIME DESTINATION .)