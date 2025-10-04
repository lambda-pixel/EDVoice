set(EDVOICE_SOURCES
    main.cpp
    EDVoiceApp.cpp
    util/EliteFileUtil.cpp
    watchers/JournalWatcher.cpp
    watchers/StatusWatcher.cpp

    voicepack/Enum.cpp
    voicepack/AudioPlayer.cpp
    voicepack/VoicePack.cpp
    voicepack/VoiceLine.cpp
    voicepack/VoicePackManager.cpp
    voicepack/MedicCompliant.cpp
    voicepack/VoicePackUtil.cpp

    ../assets/edvoice.rc
)

set(EDVOICE_SOURCES_GUI
    GUI/EDVoiceGUI.cpp
    GUI/Vulkan/VkAdapter.cpp
    GUI/Vulkan/Swapchain.cpp
    GUI/Vulkan/VkUtil.cpp

    GUI/WindowSystem.cpp
)

# GUI version
if (WIN32)
    add_executable(EDVoice WIN32 ${EDVOICE_SOURCES} ${EDVOICE_SOURCES_GUI})
else()
    add_executable(EDVoice ${EDVOICE_SOURCES} ${EDVOICE_SOURCES_GUI})
endif()

target_compile_definitions(EDVoice PRIVATE GUI_MODE)

target_include_directories(EDVoice PRIVATE ../3rdparty)
target_include_directories(EDVoice PRIVATE ../plugins/include)

target_link_libraries(EDVoice PRIVATE imgui)
target_compile_definitions(EDVoice PRIVATE UNICODE _UNICODE)
target_include_directories(EDVoice PRIVATE ${CMAKE_BINARY_DIR})

# CLI version without GUI
# add_executable(EDVoice-cli WIN32 ${EDVOICE_SOURCES})

# target_include_directories(EDVoice-cli PRIVATE ../3rdparty)
# target_include_directories(EDVoice-cli PRIVATE ../plugins/include)

# target_compile_definitions(EDVoice-cli PRIVATE UNICODE _UNICODE)

if (USE_SDL OR USE_SDL_MIXER)
    target_link_libraries(EDVoice PRIVATE SDL3::SDL3)
endif()

if (USE_SDL_MIXER)
    target_link_libraries(EDVoice PRIVATE SDL3_mixer::SDL3_mixer)
endif()

# target_compile_definitions(EDVoice PRIVATE BUILD_MEDICORP)
# target_compile_definitions(EDVoice PRIVATE VULKAN_DEBUG_LAYER)

# install(TARGETS EDVoice EDVoice-cli
        # RUNTIME DESTINATION .)
