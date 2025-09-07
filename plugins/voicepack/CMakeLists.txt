add_library(VoicePack SHARED
    VoicePack.cpp
)

target_include_directories(VoicePack PRIVATE ../include)
target_include_directories(VoicePack PRIVATE ../../3rdparty)

if(WIN32)
    set_target_properties(VoicePack PROPERTIES PREFIX "")
endif()

# plugins/
set_target_properties(VoicePack PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/plugins
)

# Install rule for CPack in plugins/
install(TARGETS VoicePack
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION plugins
)