if(APPLE)
    set(RPATH_BINARY_PATH "@loader_path")
else()
    set(RPATH_BINARY_PATH "$ORIGIN")
endif()

set(LPAC_VERSION "" CACHE STRING "Set version string for lpac")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} DIR_LPAC_SRCS)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/chip DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/notification DIR_LPAC_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/applet/profile DIR_LPAC_SRCS)

add_executable(lpac ${DIR_LPAC_SRCS} lpac.manifest)
target_link_libraries(lpac euicc-drivers lpac-utils)
target_include_directories(lpac PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_options(lpac PRIVATE -Wall -Wextra)

add_custom_target(version
    ${CMAKE_COMMAND}
    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    -D DST=${CMAKE_CURRENT_SOURCE_DIR}/version.h
    -D VERSION=${LPAC_VERSION}
    -P ${LPAC_CMAKE_MODULE_PATH}/git-version.cmake
)
add_dependencies(lpac version)

set_target_properties(lpac PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
    BUILD_RPATH "${RPATH_BINARY_PATH}"
)

if(UNIX)
    install(TARGETS lpac RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
