set(VOICEPACK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/voicepack)
set(ASSETS_DEST ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(DEFAULT_JSON ${CMAKE_CURRENT_SOURCE_DIR}/default.json)

file(GLOB_RECURSE ASSETS_FILES "${VOICEPACK_DIR}/*")

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${VOICEPACK_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/VoicePack
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DEFAULT_JSON} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/default.json
    DEPENDS ${ASSETS_FILES} ${DEFAULT_JSON}
    COMMENT "Copying assets and default.json to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

install(DIRECTORY ${VOICEPACK_DIR}/Aria
        DESTINATION config/VoicePack
        FILES_MATCHING PATTERN "*")

if(BUILD_MEDICORP)
    install(DIRECTORY ${VOICEPACK_DIR}/ALTA
            DESTINATION config/VoicePack
            FILES_MATCHING PATTERN "*")
endif()

install(FILES ${DEFAULT_JSON}
        DESTINATION config
)
