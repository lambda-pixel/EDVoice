add_library(EventLogger SHARED
    EventLogger.cpp
)

target_include_directories(EventLogger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if(WIN32)
    set_target_properties(EventLogger PROPERTIES PREFIX "")
endif()

# plugins/
set_target_properties(EventLogger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/plugins
)

# Install rule for CPack in plugins/
install(TARGETS EventLogger
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION plugins
)