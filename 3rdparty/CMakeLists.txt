add_subdirectory(imgui)

if (USE_SDL OR USE_SDL_MIXER)
    if (WIN32)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
    endif()

    add_subdirectory(SDL)
endif()

if (USE_SDL_MIXER)
    add_subdirectory(SDL_mixer)
endif()