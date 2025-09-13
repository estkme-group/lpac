if(APPLE)
    set(RPATH_BINARY_PATH "@loader_path")
else()
    set(RPATH_BINARY_PATH "$ORIGIN")
endif()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} DIR_LPAC_SRCS)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/chip DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/notification DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/profile DIR_LPAC_SRCS)

add_executable(lpac ${DIR_LPAC_SRCS} lpac.manifest)
if(WIN32)
    add_custom_command(TARGET lpac POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/lpac.manifest"
        "$<TARGET_FILE_DIR:lpac>/lpac.exe.manifest"
    )
endif()

target_link_libraries(lpac euicc-drivers lpac-utils)
target_include_directories(lpac PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_options(lpac PRIVATE -Wall -Wextra)

include(${CMAKE_SOURCE_DIR}/cmake/git-version.cmake)
if(NOT DEFINED LPAC_VERSION)
    set(LPAC_VERSION ${CMAKE_PROJECT_VERSION})
endif()
target_compile_definitions(lpac PRIVATE LPAC_VERSION="${LPAC_VERSION}")

set_target_properties(lpac PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
    BUILD_RPATH "${RPATH_BINARY_PATH}"
)

if(UNIX)
    install(TARGETS lpac RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
