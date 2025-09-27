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

    voicepack/Enum.cpp
    voicepack/AudioPlayer.cpp
    voicepack/VoicePack.cpp
    voicepack/VoicePackManager.cpp
    voicepack/MedicCompliant.cpp

    ../assets/edvoice.rc
)

target_include_directories(EDVoice PRIVATE ../3rdparty)
target_include_directories(EDVoice PRIVATE ../plugins/include)

target_link_libraries(EDVoice PRIVATE imgui)
target_compile_definitions(EDVoice PRIVATE UNICODE _UNICODE)

if (BUILD_MEDICORP)
    target_compile_definitions(EDVoice PRIVATE BUILD_MEDICORP)
    message(STATUS "Building with MediCorp support")
endif()

if (NOT WIN32)
    find_package(SDL3)
    target_link_libraries(EDVoice PRIVATE SDL3::SDL3)
endif()

# target_compile_definitions(EDVoice PRIVATE BUILD_MEDICORP)
# target_compile_definitions(EDVoice PRIVATE VULKAN_DEBUG_LAYER)

install(TARGETS EDVoice
        RUNTIME DESTINATION .)
