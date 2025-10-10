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

    GUI/Window/WindowSystem.cpp
    GUI/Window/Window.cpp
    GUI/Window/WindowMain.cpp
    GUI/Window/WindowOverlay.cpp
)

if (USE_VULKAN)
set(EDVOICE_SOURCES_GUI ${EDVOICE_SOURCES_GUI}
    GUI/Vulkan/VkAdapter.cpp
    GUI/Vulkan/Swapchain.cpp
    GUI/Vulkan/VkUtil.cpp
)
else ()
set(EDVOICE_SOURCES_GUI ${EDVOICE_SOURCES_GUI}
    GUI/DX11/DX11Adapter.cpp
)
endif()

if (WIN32)
    # GUI version
    add_executable(EDVoice WIN32 ${EDVOICE_SOURCES} ${EDVOICE_SOURCES_GUI})

    # CLI version without GUI
    add_executable(EDVoice-cli WIN32 ${EDVOICE_SOURCES})

    target_include_directories(EDVoice-cli PRIVATE ../3rdparty)
    target_include_directories(EDVoice-cli PRIVATE ../plugins/include)
    target_include_directories(EDVoice-cli PRIVATE ${CMAKE_BINARY_DIR})
    target_compile_definitions(EDVoice-cli PRIVATE UNICODE _UNICODE)
    
    install(TARGETS EDVoice-cli DESTINATION .)
else()
    # GUI version
    add_executable(EDVoice ${EDVOICE_SOURCES} ${EDVOICE_SOURCES_GUI})
endif()

target_compile_definitions(EDVoice PRIVATE GUI_MODE)

target_include_directories(EDVoice PRIVATE ../3rdparty)
target_include_directories(EDVoice PRIVATE ../plugins/include)
target_include_directories(EDVoice PRIVATE ${CMAKE_BINARY_DIR})
target_compile_definitions(EDVoice PRIVATE UNICODE _UNICODE)
target_link_libraries(EDVoice PRIVATE imgui)

if (USE_SDL OR USE_SDL_MIXER)
    target_link_libraries(EDVoice PRIVATE SDL3::SDL3)
endif()

if (USE_SDL_MIXER)
    target_link_libraries(EDVoice PRIVATE SDL3_mixer::SDL3_mixer)
endif()

install(TARGETS EDVoice DESTINATION .)

if (WIN32 AND (USE_SDL OR USE_SDL_MIXER))
    install(FILES $<TARGET_FILE:SDL3::SDL3-shared> DESTINATION .)
endif()

if (WIN32 AND USE_SDL_MIXER)
    install(FILES $<TARGET_FILE:SDL3_mixer::SDL3_mixer-shared> DESTINATION .)
endif()