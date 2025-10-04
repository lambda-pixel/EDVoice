add_subdirectory(imgui)

if (USE_SDL OR USE_SDL_MIXER)
    add_subdirectory(SDL)
endif()

if (USE_SDL_MIXER)
    add_subdirectory(SDL_mixer)
endif()