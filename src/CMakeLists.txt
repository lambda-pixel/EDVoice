add_executable(EDVoice WIN32
    main.cpp
    EDVoiceApp.cpp
    util/EliteFileUtil.cpp
    watchers/JournalWatcher.cpp
    watchers/StatusWatcher.cpp
    
    GUI/EDVoiceGUI.cpp
    GUI/Vulkan/VkAdapter.cpp
    GUI/Vulkan/Swapchain.cpp
    GUI/Vulkan/VkUtil.cpp
    ../assets/edvoice.rc
 )

target_include_directories(EDVoice PRIVATE ../3rdparty)
target_include_directories(EDVoice PRIVATE ../plugins/include)

target_link_libraries(EDVoice PRIVATE imgui)
target_compile_definitions(EDVoice PRIVATE UNICODE _UNICODE)

install(TARGETS EDVoice
        RUNTIME DESTINATION .)
