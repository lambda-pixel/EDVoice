set(VOICEPACK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/voicepack)
set(ASSETS_DEST ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(DEFAULT_JSON ${CMAKE_CURRENT_SOURCE_DIR}/default.json)
set(LICENCE ${VOICEPACK_DIR}/LICENCE.txt)
set(VOICEPACK_JSON ${VOICEPACK_DIR}/config.json)

file(GLOB_RECURSE ASSETS_FILES "${VOICEPACK_DIR}/*")

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${VOICEPACK_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/VoicePack
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DEFAULT_JSON} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/default.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LICENCE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/LICENCE.txt
    DEPENDS ${ASSETS_FILES} ${DEFAULT_JSON}
    COMMENT "Copying assets and default.json to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

install(DIRECTORY "${VOICEPACK_DIR}/[FR] Anne"
        DESTINATION config/VoicePack
        FILES_MATCHING PATTERN "*")

install(DIRECTORY "${VOICEPACK_DIR}/[FR] Aria"
        DESTINATION config/VoicePack
        FILES_MATCHING PATTERN "*")

if(BUILD_MEDICORP)
    install(DIRECTORY "${VOICEPACK_DIR}/ALTA"
            DESTINATION config/VoicePack
            FILES_MATCHING PATTERN "*")
endif()

install(FILES 
    ${DEFAULT_JSON}
    DESTINATION config
)

install(FILES 
    ${VOICEPACK_JSON}
    DESTINATION config/VoicePack
)

install(FILES 
    ${LICENCE}
    DESTINATION config/VoicePack
)